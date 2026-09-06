cmake_minimum_required(VERSION 3.12.4)

# Migrations reach the models through libapp.so, the same single instance the
# handlers use -- `migrate` loads it from `main.modules` before running them.
cwfr_add_migrations(LINK_LIBS cwfr::framework app)
