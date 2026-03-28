use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int};

// link to C++ login and exit idk funcs they are in runtime.h?? wait let me check actually it might be rcoket_cli.h i kinda forgot
// TODO: Rewrite this file in python so i can read
extern "C" {
    fn rs_rocket_log(log: *const c_char, source: *const c_char, func: *const c_char, level: *const c_char);
    fn rs_rocket_exit(status_code: c_int);
}

#[repr(C)]
pub struct rs_RocketCliResult {
    pub noplugins: bool,
    pub debugoverlay: bool,
    pub nosplash: bool,
    pub notext: bool,

    pub forcewayland: bool,
    pub software_frame_timer: bool,
    pub viewport_size_set: bool,
    pub should_exit: bool,

    pub glversion: i32,
    pub framerate: i32,
    pub exit_code: i32,

    pub viewport_size: [u8; 64],
    pub logfile: [u8; 256],
}

#[no_mangle]
pub extern "C" fn rs_parse_rocket_arguments(argc: c_int, argv: *const *const c_char) -> rs_RocketCliResult {
    let mut res = rs_RocketCliResult {
        noplugins: false,
        debugoverlay: false,
        nosplash: false,
        notext: false,

        forcewayland: false,
        software_frame_timer: false,
        viewport_size_set: false,
        should_exit: false,

        glversion: 0,
        framerate: -1,
        exit_code: 0,

        viewport_size: [0; 64],
        logfile: [0; 256],
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
                println!(r#"Usage: TODO:argv0 [options]

Format: [--arg] or [-arg] or [/arg]

Arguments:
    no-plugins, noplugins
    -> disable all plugins before startup

*   logfile [file_path] -- TODO:IMPLEMENT
    -> logs messages to specified file path

    debug-overlay, debugoverlay, doverlay
    -> shows a debug overlay with debugging information

*   gl-version, glversion [3.3 -> 4.6]
    -> forces an OpenGL version to be used

    no-splash, nosplash
    -> hides splash from being shown in the beginning

*   viewport-size, viewportsize, vp-size, vpsize [width]x[height]
    -> forces to use a initial viewport (and window) size

*   framerate [fps]
    -> force renderer to use a target framerate (if reachable)

    version
    -> show version and attribution

* | Value Mandatory

--> Powered by RocketGE
"#);
                res.should_exit = true;
            }
            _ => {
            }
        }
    }

    res
}
