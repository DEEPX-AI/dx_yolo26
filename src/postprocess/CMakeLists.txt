# Postprocess shared libraries. Kept as a separate library so that other DEEPX
# components (dx_stream, the python bindings) can reuse the YOLO26 postprocessing
# without linking the example applications.
add_subdirectory(yolov26)
