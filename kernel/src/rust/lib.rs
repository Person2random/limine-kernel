#![no_std]
use core::panic::PanicInfo;

mod ps2_keyboard;
mod ps2_mouse;

#[no_mangle]
pub extern "C" fn rust_test() -> u64 {
    0xDEADBEEF
}

extern "C" {
    fn kernel_panic(message: *const core::ffi::c_char, code: u16) -> !;
}
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    unsafe {
        kernel_panic(_info.message().as_str().unwrap().as_ptr().cast::<i8>(), 0);
    }
}
