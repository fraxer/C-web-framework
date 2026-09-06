cmake_minimum_required(VERSION 3.12.4)

# cwfr::framework rather than the individual core archives (model, database, ...):
# it propagates every core include directory through its INTERFACE while
# propagating no static link requirement, so nothing from the core is duplicated
# into libapp.so.
cwfr_add_lib(models LINK_LIBS cwfr::framework)
