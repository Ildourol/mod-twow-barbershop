# Optional CMake hooks for mod-twow-barbershop.
#
# Most modules do not need custom CMake here. Source files under src/ are discovered automatically,
# conf/*.conf.dist files are copied automatically, and Addmod_twow_barbershopScripts() is invoked by the
# generated module loader.
#
# This file is included twice:
#   TORTOISE_MODULE_CMAKE_PHASE=DISCOVERY
#   TORTOISE_MODULE_CMAKE_PHASE=POST_TARGETS
#
# Use TW_* helpers for compatibility with this core's module system. Do not use AzerothCore AC_* names.

if(TORTOISE_MODULE_CMAKE_PHASE STREQUAL "DISCOVERY")
  # Example for legacy-style extra script sources:
  # TW_ADD_SCRIPTS("${CMAKE_CURRENT_LIST_DIR}/src/legacy")
  # TW_ADD_SCRIPT_LOADER(mod_twow_barbershop "legacy_loader.h")
endif()

# NOTE: Compilation is handled exclusively by the operator/user.
# Any optional test targets or standalone builds should be OFF by default.
