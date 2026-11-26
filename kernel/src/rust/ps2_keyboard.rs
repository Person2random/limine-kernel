// When I started writing this, Only God and I knew how it worked. Now, Only God knows.
// Yes I stole the quote from Martin Fowler, but I think it fits here too.
use core::sync::atomic::{AtomicUsize, Ordering};

const DATA_PORT: u16 = 0x60;
const STATUS_PORT: u16 = 0x64;
const COMMAND_PORT: u16 = 0x64;
const PIC1_COMMAND: u16 = 0x20;
const PIC1_DATA: u16 = 0x21;
const PIC2_COMMAND: u16 = 0xA0;
const PIC2_DATA: u16 = 0xA1;
const PIC_EOI: u8 = 0x20;
const PIC1_VECTOR_OFFSET: u8 = 0x40;
const PIC2_VECTOR_OFFSET: u8 = 0x48;
const QUEUE_SIZE: usize = 128;
const QUEUE_MASK: usize = QUEUE_SIZE - 1;
const CONTROLLER_TIMEOUT: usize = 100_000;

static mut EVENTS: [u16; QUEUE_SIZE] = [0; QUEUE_SIZE];
static HEAD: AtomicUsize = AtomicUsize::new(0);
static TAIL: AtomicUsize = AtomicUsize::new(0);
static DROPPED_EVENTS: AtomicUsize = AtomicUsize::new(0);
static mut EXTENDED_SCANCODE: bool = false;
static mut PAUSE_SCANCODES_REMAINING: u8 = 0;

#[inline]
unsafe fn inb(port: u16) -> u8 {
    let value: u8;
    core::arch::asm!(
        "in al, dx",
        in("dx") port,
        out("al") value,
        options(nomem, nostack, preserves_flags)
    );
    value
}

#[inline]
unsafe fn outb(port: u16, value: u8) {
    core::arch::asm!(
        "out dx, al",
        in("dx") port,
        in("al") value,
        options(nomem, nostack, preserves_flags)
    );
}

fn wait_input_empty() -> bool {
    for _ in 0..CONTROLLER_TIMEOUT {
        if unsafe { inb(STATUS_PORT) } & 0x02 == 0 {
            return true;
        }
        core::hint::spin_loop();
    }
    false
}

fn wait_output_full() -> bool {
    for _ in 0..CONTROLLER_TIMEOUT {
        if unsafe { inb(STATUS_PORT) } & 0x01 != 0 {
            return true;
        }
        core::hint::spin_loop();
    }
    false
}

fn controller_command(command: u8) -> bool {
    if !wait_input_empty() {
        return false;
    }
    unsafe { outb(COMMAND_PORT, command) };
    true
}

fn keyboard_command(command: u8) -> bool {
    if !wait_input_empty() {
        return false;
    }
    unsafe { outb(DATA_PORT, command) };
    for _ in 0..3 {
        if !wait_output_full() {
            return false;
        }
        let response = unsafe { inb(DATA_PORT) };
        match response {
            0xFA => return true,
            0xFE => {
                if !wait_input_empty() {
                    return false;
                }
                unsafe { outb(DATA_PORT, command) };
            }
            _ => {}
        }
    }
    false
}

fn remap_pic_for_keyboard() {
    unsafe {
        outb(PIC1_DATA, 0xFF);
        outb(PIC2_DATA, 0xFF);
    }

    unsafe {
        outb(PIC1_COMMAND, 0x11);
        outb(0x80, 0);
        outb(PIC2_COMMAND, 0x11);
        outb(0x80, 0);
        outb(PIC1_DATA, PIC1_VECTOR_OFFSET);
        outb(0x80, 0);
        outb(PIC2_DATA, PIC2_VECTOR_OFFSET);
        outb(0x80, 0);
        outb(PIC1_DATA, 0x04);
        outb(0x80, 0);
        outb(PIC2_DATA, 0x02);
        outb(0x80, 0);
        outb(PIC1_DATA, 0x01);
        outb(0x80, 0);
        outb(PIC2_DATA, 0x01);
        outb(0x80, 0);
        outb(PIC1_DATA, 0xFD);
        outb(PIC2_DATA, 0xFF);
    }
}

fn flush_output_buffer() -> bool {
    for _ in 0..CONTROLLER_TIMEOUT {
        let status = unsafe { inb(STATUS_PORT) };
        if status & 1 == 0 {
            return true;
        }
        unsafe { inb(DATA_PORT) };
    }
    false
}

#[no_mangle]
pub extern "C" fn ps2_keyboard_init() -> bool {
    unsafe {
        outb(PIC1_DATA, 0xFF);
        outb(PIC2_DATA, 0xFF);
    }

    if !controller_command(0xAD) {
        return false;
    }
    if !flush_output_buffer() {
        return false;
    }

    if !controller_command(0x20) || !wait_output_full() {
        return false;
    }
    let mut command_byte = unsafe { inb(DATA_PORT) };
    command_byte |= 0x01;
    command_byte &= !0x12;
    if !controller_command(0x60) || !wait_input_empty() {
        return false;
    }
    unsafe { outb(DATA_PORT, command_byte) };

    if !controller_command(0xAE) || !keyboard_command(0xF4) {
        return false;
    }
    if !flush_output_buffer() {
        return false;
    }
    remap_pic_for_keyboard();

    HEAD.store(0, Ordering::Relaxed);
    TAIL.store(0, Ordering::Relaxed);
    DROPPED_EVENTS.store(0, Ordering::Relaxed);
    true
}

#[no_mangle]
pub extern "C" fn ps2_keyboard_irq() {
    let status = unsafe { inb(STATUS_PORT) };
    if status & 1 != 0 {
        let scancode = unsafe { inb(DATA_PORT) };
        if status & 0x20 == 0 && status & 0xC0 == 0 {
            unsafe {
                if PAUSE_SCANCODES_REMAINING != 0 {
                    PAUSE_SCANCODES_REMAINING -= 1;
                } else if scancode == 0xE0 {
                    EXTENDED_SCANCODE = true;
                } else if scancode == 0xE1 {
                    EXTENDED_SCANCODE = false;
                    PAUSE_SCANCODES_REMAINING = 5;
                } else {
                    let extended = EXTENDED_SCANCODE;
                    EXTENDED_SCANCODE = false;
                    let pressed = scancode & 0x80 == 0;
                    let event = (scancode & 0x7F) as u16
                        | if extended { 0x0100 } else { 0 }
                        | if pressed { 0x8000 } else { 0 };
                    let head = HEAD.load(Ordering::Relaxed);
                    let next_head = (head + 1) & QUEUE_MASK;
                    if next_head == TAIL.load(Ordering::Acquire) {
                        DROPPED_EVENTS.fetch_add(1, Ordering::Relaxed);
                    } else {
                        core::ptr::addr_of_mut!(EVENTS)
                            .cast::<u16>()
                            .add(head)
                            .write(event);
                        HEAD.store(next_head, Ordering::Release);
                    }
                }
            }
        }
    }
    unsafe { outb(PIC1_COMMAND, PIC_EOI) };
}

#[no_mangle]
pub extern "C" fn ps2_keyboard_read_event() -> u16 {
    let tail = TAIL.load(Ordering::Relaxed);
    if tail == HEAD.load(Ordering::Acquire) {
        return 0;
    }

    let event = unsafe {
        core::ptr::addr_of!(EVENTS)
            .cast::<u16>()
            .add(tail)
            .read()
    };
    TAIL.store((tail + 1) & QUEUE_MASK, Ordering::Release);
    event
}

// Returns 0 if empty; otherwise bit 15 is pressed, bit 8 is E0-prefixed,
// and bits 0-7 contain the set-1 scan code.
// I also don't really know what this does, I stole it off of a website but it seems to work, so I guess it does what it says on the tin.
#[no_mangle]
pub extern "C" fn ps2_keyboard_dropped_events() -> usize {
    DROPPED_EVENTS.load(Ordering::Relaxed)
}
