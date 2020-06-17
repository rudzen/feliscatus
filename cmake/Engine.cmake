# Contains various settings for the engine when it is compiled.
function(enable_engine_options)
    option(NO_EVAL_LAZY_THRESHOLD "Enable lazy (value) evaluation threshold" OFF)
    if (NO_EVAL_LAZY_THRESHOLD)
        add_definitions("-DNO_LAZY_EVAL_THRESHOLD")
    endif()

    option(NO_PREFETCH "Disables L1/L2 data preloading" OFF)
    if (NO_PREFETCH)
        add_definitions("-DNO_PREFETCH")
    endif()

endfunction()
