# MoltenVKとその依存をアプリ内へコピーし、SDKのないMacでもロード可能にする
include(BundleUtilities)
get_filename_component(moltenvk_directory "${MOLTENVK_LIBRARY}" DIRECTORY)
fixup_bundle("${BUNDLE}" "${MOLTENVK_LIBRARY}" "${moltenvk_directory}")
execute_process(COMMAND codesign --force --deep --sign - "${BUNDLE}" COMMAND_ERROR_IS_FATAL ANY)
