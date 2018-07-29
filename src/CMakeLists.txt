
set(hogtess_SOURCES
    input/input.hpp
    input/input-mfem.cpp
    input/input-mfem.hpp
    surface/surface.cpp
    surface/surface.hpp
    shape/shape.cpp
    shape/shape.hpp
    main.cpp
    main.hpp
    render.cpp
    render.hpp
    palette.hpp
    shader.hpp
)

set(hogtess_MOC_HEADERS
    main.hpp
    render.hpp
)

qt4_wrap_cpp(hogtess_MOC_FILES ${hogtess_MOC_HEADERS})

file_to_cpp(hogtess_DATA shaders::shape shape/shape.glsl)
file_to_cpp(hogtess_DATA shaders::surface::tesselate surface/tesselate.glsl)
file_to_cpp(hogtess_DATA shaders::surface::draw surface/draw.glsl)
file_to_cpp(hogtess_DATA shaders::surface::lines surface/lines.glsl)

add_executable(hogtess
    ${hogtess_SOURCES}
    ${hogtess_MOC_FILES}
    ${hogtess_DATA}
)

target_link_libraries(hogtess
    ${QT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${MFEM_PATH}/libmfem.a
)
