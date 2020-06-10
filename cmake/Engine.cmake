# Contains various settings for the engine when it is compiled.
function(enable_engine_options)
    option(ENABLE_CUCKOO "Enables the use of the cuckoo algorithm" ON)
    if (ENABLE_CUCKOO)
        add_definitions("-DCUCKOO")
        message("CUCKOO enabled")
    else()
        message("CUCKOO disabled")
    endif()

    option(ENABLE_END_GAME_TABLE "Enables the use of syzygy end game tables" ON)
    if (ENABLE_END_GAME_TABLE)
        add_definitions("-DEGTABLE")
    endif()

    option(ENABLE_DEBUG_EVAL_SIMMETRY "Enables debugging with board simmetry" OFF)
    if (ENABLE_DEBUG_EVAL_SIMMETRY)
        add_definitions("-DDEBUG_EVAL_SIMMETRY")
    endif()

    option(DISABLE_TIME_DIPENDENT_OUTPUT "Disables time dependant output" OFF)
    if (DISABLE_TIME_DIPENDENT_OUTPUT)
        add_definitions("-DDISABLE_TIME_DIPENDENT_OUTPUT")
    endif()

    option(ENABLE_CHECK_CONSISTENCY "Enables position consitency verificationb" OFF)
    if (ENABLE_CHECK_CONSISTENCY)
        add_definitions("-DENABLE_CHECK_CONSISTENCY")
    endif()

    option(EVAL_LAZY_THRESHOLD "Enable lazy (value) evaluation threshold" OFF)
    if (EVAL_LAZY_THRESHOLD)
        add_definitions("-DLAZY_EVAL_THRESHOLD")
    endif()

    option(EVAL_THREAT_ON_QUEEN "Enables threat on queen evaluationb" OFF)
    if (EVAL_THREAT_ON_QUEEN)
        add_definitions("-DTHREAT_ON_QUEEN")
    endif()

    option(EVAL_STRONGLY_PROTECTED_MINOR "Enables strongly protected minor evaluation" OFF)
    if (EVAL_STRONGLY_PROTECTED_MINOR)
        add_definitions("-DSTRONGLY_PROTECTED_MINOR")
    endif()

    option(ENABLE_THREAD_VOTING "Enables thread voting for best move found across threads" ON)
    if (ENABLE_THREAD_VOTING)
        add_definitions("-DENABLE_THREAD_VOTING")
    endif()

endfunction()
