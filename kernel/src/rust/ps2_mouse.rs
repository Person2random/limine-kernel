use core::sync::atomic::{AtomicUsize, Ordering};

const DATA_PORT: u16 = 0x60;
const STATUS_PORT: u16 = 0x64;
const COMMAND_PORT: u16 = 0x64;
const PIC1_COMMAND: u16 = 0x20;
const PIC1_DATA: u16 = 0x21;
const PIC2_COMMAND: u16 = 0xA0;
const PIC2_DATA: u16 = 0xA1;
const QUEUE_SIZE: usize = 64;
const QUEUE_MASK: usize = QUEUE_SIZE - 1;
const CONTROLLER_TIMEOUT: usize = 100_000;

static mut EVENTS: [u32; QUEUE_SIZE] = [0; QUEUE_SIZE];
static HEAD: AtomicUsize = AtomicUsize::new(0);
static TAIL: AtomicUsize = AtomicUsize::new(0);
static mut PACKET: [u8; 3] = [0; 3];
static mut PACKET_INDEX: usize = 0;

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

fn mouse_command(command: u8) -> bool {
    for _ in 0..3 {
        if !controller_command(0xD4) || !wait_input_empty() {
            return false;
        }
        unsafe { outb(DATA_PORT, command) };
        if !wait_output_full() {
            return false;
        }
        match unsafe { inb(DATA_PORT) } {
            0xFA => return true,
            0xFE => continue,
            _ => return false,
        }
    }
    false
}

#[no_mangle]
pub extern "C" fn ps2_mouse_init() -> bool {
    if !controller_command(0xA8) || !controller_command(0x20) ||
        !wait_output_full() {
        return false;
    }
    let mut command_byte = unsafe { inb(DATA_PORT) };
    command_byte |= 0x02;
    command_byte &= !0x20;
    if !controller_command(0x60) || !wait_input_empty() {
        return false;
    }
    unsafe { outb(DATA_PORT, command_byte) };

    if !mouse_command(0xF6) || !mouse_command(0xF4) {
        return false;
    }

    unsafe {
        outb(PIC1_DATA, 0xF9);
        outb(PIC2_DATA, 0xEF);
    }
    HEAD.store(0, Ordering::Relaxed);
    TAIL.store(0, Ordering::Relaxed);
    unsafe {
        PACKET_INDEX = 0;
    }
    true
}

#[no_mangle]
pub extern "C" fn ps2_mouse_irq() {
    let status = unsafe { inb(STATUS_PORT) };
    if status & 0x21 == 0x21 && status & 0xC0 == 0 {
        let byte = unsafe { inb(DATA_PORT) };
        let packet_index = unsafe { PACKET_INDEX };
        if packet_index == 0 && byte & 0x08 == 0 {
            unsafe {
                outb(PIC2_COMMAND, 0x20);
                outb(PIC1_COMMAND, 0x20);
            }
            return;
        }

        unsafe {
            PACKET[packet_index] = byte;
            PACKET_INDEX += 1;
            if PACKET_INDEX == 3 {
                PACKET_INDEX = 0;
                let flags = PACKET[0];
                if flags & 0xC0 == 0 {
                    let dx = PACKET[1];
                    let dy = PACKET[2];
                    let buttons = flags & 0x07;
                    let event = 0x8000_0000
                        | ((buttons as u32) << 16)
                        | ((dx as u32) << 8)
                        | dy as u32;
                    let head = HEAD.load(Ordering::Relaxed);
                    let next_head = (head + 1) & QUEUE_MASK;
                    if next_head != TAIL.load(Ordering::Acquire) {
                        core::ptr::addr_of_mut!(EVENTS)
                            .cast::<u32>()
                            .add(head)
                            .write(event);
                        HEAD.store(next_head, Ordering::Release);
                    }
                }
            }
        }
    } else if status & 1 != 0 {
        unsafe { inb(DATA_PORT) };
    }
    unsafe {
        outb(PIC2_COMMAND, 0x20);
        outb(PIC1_COMMAND, 0x20);
    }
}

#[no_mangle]
pub extern "C" fn ps2_mouse_read_event() -> u32 {
    let tail = TAIL.load(Ordering::Relaxed);
    if tail == HEAD.load(Ordering::Acquire) {
        return 0;
    }
    let event = unsafe {
        core::ptr::addr_of!(EVENTS)
            .cast::<u32>()
            .add(tail)
            .read()
    };
    TAIL.store((tail + 1) & QUEUE_MASK, Ordering::Release);
    event
}
