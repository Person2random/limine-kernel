#![no_std]
use core::panic::PanicInfo;

#[no_mangle]
pub extern "C" fn rust_test() -> u64 {
    0xDEADBEEF
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

