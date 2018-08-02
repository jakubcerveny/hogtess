
set(hogtess_SOURCES
    cutplane/cutmesh.cpp
    cutplane/cutmesh.hpp
    cutplane/draw.glsl
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
    palette.cpp
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

file_to_cpp(hogtess_DATA shaders::cutplane::voxelize cutplane/voxelize.glsl)
file_to_cpp(hogtess_DATA shaders::cutplane::march cutplane/march.glsl)
file_to_cpp(hogtess_DATA shaders::cutplane::draw cutplane/draw.glsl)

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
