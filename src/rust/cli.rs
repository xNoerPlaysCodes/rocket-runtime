use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int};

// link to C++ login and exit idk funcs they are in runtime.h?? wait let me check actually it might be rcoket_cli.h i kinda forgot
// TODO: Rewrite this file in python so i can read
extern "C" {
    fn rs_rocket_log(log: *const c_char, source: *const c_char, func: *const c_char, level: *const c_char);
    fn rs_rocket_exit(status_code: c_int);
}

#[repr(C)]
pub struct RocketCliResult {
    pub noplugins: bool, pub logall: bool, pub debugoverlay: bool,
    pub nosplash: bool, pub notext: bool, pub forcewayland: bool,
    pub software_frame_timer: bool, pub glversion: i32, pub framerate: i32,
    pub viewport_size: [u8; 64], pub viewport_size_set: bool,
    pub should_exit: bool, pub exit_code: i32,
}

#[no_mangle]
pub extern "C" fn parse_rocket_arguments(argc: c_int, argv: *const *const c_char) -> RocketCliResult {
    let mut res = RocketCliResult {
        noplugins: false, logall: false, debugoverlay: false, nosplash: false,
        notext: false, forcewayland: false, software_frame_timer: false,
        glversion: 0, framerate: -1, viewport_size: [0; 64],
        viewport_size_set: false, should_exit: false, exit_code: 0,
    };

    let args: Vec<String> = unsafe {
        std::slice::from_raw_parts(argv, argc as usize)
            .iter()
            .map(|&p| CStr::from_ptr(p).to_string_lossy().into_owned())
            .collect()
    };

    // LOGGING FROM RUST
    let msg = CString::new("CLI Parser Initialized").unwrap();
    let src = CString::new("cli").unwrap();
    let fun = CString::new("parser").unwrap();
    let lvl = CString::new("debug").unwrap();
    unsafe { rs_rocket_log(msg.as_ptr(), src.as_ptr(), fun.as_ptr(), lvl.as_ptr()); }

    for (i, arg) in args.iter().enumerate() {
        match arg.trim_start_matches(|c| c == '-' || c == '/').as_ref() {
            "nosplash" | "no-splash" => res.nosplash = true,
            "noplugins" => res.noplugins = true,
            "framerate" => {
                if let Some(val) = args.get(i + 1) {
                    res.framerate = val.parse().unwrap_or(-1);
                }
            }
            "help" => {
                println!("Rocket-GE is a blazingly fast game framework, with some parts of it even written in rust!!!\n--nosplash\n--noplugins\n--framerate [fps]");
                res.should_exit = true;
            }
            _ => {}
        }
    }

    res
}
