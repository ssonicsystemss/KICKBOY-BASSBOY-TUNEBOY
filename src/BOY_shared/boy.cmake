# BOY series (KICKBOY / BASSBOY / TUNEBOY) by sonicsystems: one shared framework, never copied. In a plugin's
# CMakeLists.txt:
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../../BOY_shared/boy.cmake)
#   boy_use(<plugin target>)      # and the same for the render test tool
# Adds the include paths (BOY_shared + SONICSYSTEMS_shared for OWN SOUND) and the two embedded pixel fonts
# (Silkscreen for labels + VT323 for readouts, both subset to Latin, OFL, see fonts/OFL-*.txt).

set(BOY_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(boy_use target)
    target_include_directories(${target} PRIVATE "${BOY_DIR}" "${BOY_DIR}/../SONICSYSTEMS_shared")
    target_compile_definitions(${target} PRIVATE JUCE_USE_MP3AUDIOFORMAT=1)
    if(NOT TARGET BoyFonts)
        juce_add_binary_data(BoyFonts HEADER_NAME BoyFonts.h NAMESPACE BoyFonts
            SOURCES "${BOY_DIR}/fonts/Silkscreen-Sub.ttf" "${BOY_DIR}/fonts/VT323-Sub.ttf")
        set_target_properties(BoyFonts PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
    endif()
    target_link_libraries(${target} PRIVATE BoyFonts)
endfunction()
