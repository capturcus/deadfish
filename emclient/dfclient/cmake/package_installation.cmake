set(RUNTIME_INSTALL_DESTINATION bin)
set(LIBRARY_INSTALL_DESTINATION lib)

if(MSVC OR APPLE OR EMSCRIPTEN)
	set(README_INSTALL_DESTINATION .)
	set(DATA_INSTALL_DESTINATION data)
	set(SHADERS_INSTALL_DESTINATION data/shaders)
else()
	set(README_INSTALL_DESTINATION share/${PACKAGE_LOWER_NAME})
	set(DATA_INSTALL_DESTINATION share/${PACKAGE_LOWER_NAME}/data)
	set(SHADERS_INSTALL_DESTINATION share/${PACKAGE_LOWER_NAME}/shaders)
endif()

set(CPACK_PACKAGE_NAME "${PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR ${PACKAGE_VENDOR})
set(CPACK_PACKAGE_DESCRIPTION ${PACKAGE_DESCRIPTION})
set(CPACK_PACKAGE_HOMEPAGE_URL ${PACKAGE_HOMEPAGE})
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PACKAGE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PACKAGE_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PACKAGE_NAME}")
if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
set(CPACK_PACKAGE_CHECKSUM MD5)

if(MSVC)
	set(CPACK_GENERATOR NSIS ZIP)
	set(CPACK_NSIS_MUI_ICON "${PACKAGE_DATA_DIR}/icons/icon.ico")
	set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
	# Custom NSIS commands needed in order to set the "Start in" property of the start menu shortcut
	set(CPACK_NSIS_CREATE_ICONS_EXTRA
		"SetOutPath '$INSTDIR\\\\bin'
		CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${PACKAGE_NAME}.lnk' '$INSTDIR\\\\bin\\\\${PACKAGE_EXE_NAME}.exe'
		CreateShortCut '$DESKTOP\\\\${PACKAGE_NAME}.lnk' '$INSTDIR\\\\bin\\\\${PACKAGE_EXE_NAME}.exe'
		SetOutPath '$INSTDIR'")
	set(CPACK_NSIS_DELETE_ICONS_EXTRA
		"Delete '$SMPROGRAMS\\\\$MUI_TEMP\\\\${PACKAGE_NAME}.lnk'
		Delete '$DESKTOP\\\\${PACKAGE_NAME}.lnk'")

	if(PACKAGE_INSTALL_SYSLIBS)
		include(InstallRequiredSystemLibraries)
	endif()

	set(PACKAGE_SYSTEM_NAME "Win64")
	if("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 16 2019")
		set(PACKAGE_COMPILER "VS2019")
	elseif("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 15 2017")
		set(PACKAGE_COMPILER "VS2017")
	endif()
elseif(APPLE)
	set(CPACK_GENERATOR "Bundle")
	set(CPACK_BUNDLE_NAME ${PACKAGE_NAME})
	set(FRAMEWORKS_INSTALL_DESTINATION ../Frameworks)

	configure_file(${CMAKE_SOURCE_DIR}/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
	set(CPACK_BUNDLE_PLIST ${CMAKE_BINARY_DIR}/Info.plist)

	file(RELATIVE_PATH RELPATH_TO_BIN ${CMAKE_INSTALL_PREFIX}/MacOS ${CMAKE_INSTALL_PREFIX}/Resources/${RUNTIME_INSTALL_DESTINATION})
	file(WRITE ${CMAKE_BINARY_DIR}/bundle_executable "#!/usr/bin/env sh\ncd \"$(dirname \"$0\")\" \ncd ${RELPATH_TO_BIN} && ./${PACKAGE_EXE_NAME}")
	install(FILES ${CMAKE_BINARY_DIR}/bundle_executable DESTINATION ../MacOS/ RENAME ${CPACK_BUNDLE_NAME}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

	add_custom_command(
		OUTPUT ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PACKAGE_DATA_DIR}/icons/icon1024.png ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_512x512@2x.png
		COMMAND sips -z 512 512 ${PACKAGE_DATA_DIR}/icons/icon1024.png --out ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_512x512.png
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_512x512.png ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_256x256@2x.png
		COMMAND sips -z 256 256 ${PACKAGE_DATA_DIR}/icons/icon1024.png --out ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_256x256.png
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_256x256.png ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_128x128@2x.png
		COMMAND sips -z 128 128 ${PACKAGE_DATA_DIR}/icons/icon1024.png --out ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_128x128.png
		COMMAND sips -z 64 64 ${PACKAGE_DATA_DIR}/icons/icon1024.png --out ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_32x32@2x.png
		COMMAND sips -z 32 32 ${PACKAGE_DATA_DIR}/icons/icon1024.png --out ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_32x32.png
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_32x32.png ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_16x16@2x.png
		COMMAND sips -z 16 16 ${PACKAGE_DATA_DIR}/icons/icon1024.png --out ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset/icon_16x16.png
		COMMAND iconutil --convert icns --output ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.icns ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset)
	add_custom_target(iconutil_convert ALL DEPENDS ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.iconset)
	set(CPACK_BUNDLE_ICON ${CMAKE_BINARY_DIR}/${CPACK_BUNDLE_NAME}.icns)
elseif(EMSCRIPTEN)
	if(CMAKE_HOST_WIN32)
		set(CPACK_GENERATOR ZIP)
	else()
		set(CPACK_GENERATOR TGZ)
	endif()
elseif(UNIX AND NOT APPLE)
	set(CPACK_GENERATOR TGZ)
	set(ICONS_INSTALL_DESTINATION share/icons/hicolor)

	if(EXISTS ${PACKAGE_DATA_DIR}/icons)
		install(FILES ${PACKAGE_DATA_DIR}/icons/icon1024.png DESTINATION ${ICONS_INSTALL_DESTINATION}/1024x1024/apps/ RENAME ${PACKAGE_NAME}.png)
		install(FILES ${PACKAGE_DATA_DIR}/icons/icon192.png DESTINATION ${ICONS_INSTALL_DESTINATION}/192x192/apps/ RENAME ${PACKAGE_NAME}.png)
		install(FILES ${PACKAGE_DATA_DIR}/icons/icon96.png DESTINATION ${ICONS_INSTALL_DESTINATION}/96x96/apps/ RENAME ${PACKAGE_NAME}.png)
		install(FILES ${PACKAGE_DATA_DIR}/icons/icon72.png DESTINATION ${ICONS_INSTALL_DESTINATION}/72x72/apps/ RENAME ${PACKAGE_NAME}.png)
		install(FILES ${PACKAGE_DATA_DIR}/icons/icon48.png DESTINATION ${ICONS_INSTALL_DESTINATION}/48x48/apps/ RENAME ${PACKAGE_NAME}.png)
	endif()

	configure_file(${CMAKE_SOURCE_DIR}/package.desktop ${CMAKE_BINARY_DIR}/${PACKAGE_DESKTOP_FILE} @ONLY)
	install(FILES ${CMAKE_BINARY_DIR}/${PACKAGE_DESKTOP_FILE} DESTINATION share/applications)

	set(PACKAGE_SYSTEM_NAME "Linux")
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(PACKAGE_COMPILER "GCC")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(PACKAGE_COMPILER "Clang")
	endif()
elseif(MINGW)
	set(CPACK_GENERATOR TGZ)

	set(PACKAGE_SYSTEM_NAME "MinGW")
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(PACKAGE_COMPILER "GCC")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(PACKAGE_COMPILER "Clang")
	endif()
endif()

# Custom override of CPack package name
if(DEFINED PACKAGE_SYSTEM_NAME)
	if(DEFINED PACKAGE_COMPILER)
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${PACKAGE_SYSTEM_NAME}-${PACKAGE_COMPILER})
	else()
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${PACKAGE_SYSTEM_NAME})
	endif()
endif()

include(CPack)

install(TARGETS ${PACKAGE_EXE_NAME} RUNTIME DESTINATION ${RUNTIME_INSTALL_DESTINATION})
install(FILES README.md DESTINATION ${README_INSTALL_DESTINATION})
if((MSVC OR APPLE) AND EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
	install(FILES LICENSE DESTINATION . RENAME LICENSE.txt)
endif()

if(NOT EMSCRIPTEN)
	install(DIRECTORY ${PACKAGE_DATA_DIR}/data/ DESTINATION ${DATA_INSTALL_DESTINATION})
else()
	install(FILES ${CMAKE_BINARY_DIR}/${PACKAGE_EXE_NAME}.html DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	install(FILES ${CMAKE_BINARY_DIR}/${PACKAGE_EXE_NAME}.data DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	install(FILES ${CMAKE_BINARY_DIR}/${PACKAGE_EXE_NAME}.wasm DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	if(EXISTS ${CMAKE_BINARY_DIR}/favicon.ico)
		install(FILES ${CMAKE_BINARY_DIR}/favicon.ico DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	endif()
endif()
install(FILES ${PACKAGE_DATA_DIR}/README.md DESTINATION ${DATA_INSTALL_DESTINATION})
if(IS_DIRECTORY ${NCINE_SHADERS_DIR})
	install(DIRECTORY ${NCINE_SHADERS_DIR} DESTINATION ${SHADERS_INSTALL_DESTINATION})
endif()

if(MSVC)
	if(NCINE_DYNAMIC_LIBRARY)
		install(FILES ${NCINE_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	endif()

	if(EXISTS ${NCINE_CONFIG_H})
		if(ANGLE_FOUND AND NCINE_WITH_ANGLE)
			get_target_property(EGL_IMPORTED_LOCATION EGL::EGL IMPORTED_LOCATION)
			install(FILES ${EGL_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
			get_target_property(GLES_IMPORTED_LOCATION OpenGLES::GLES IMPORTED_LOCATION)
			install(FILES ${GLES_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
		endif()
		if(GLEW_FOUND AND NCINE_WITH_GLEW)
			get_target_property(GLEW_IMPORTED_LOCATION GLEW::GLEW IMPORTED_LOCATION)
			install(FILES ${GLEW_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
		endif()
		if(GLFW_FOUND AND NCINE_WITH_GLFW)
			get_target_property(GLFW_IMPORTED_LOCATION GLFW::GLFW IMPORTED_LOCATION)
			install(FILES ${GLFW_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
		endif()
		if(SDL2_FOUND AND NCINE_WITH_SDL)
			get_target_property(SDL2_IMPORTED_LOCATION SDL2::SDL2 IMPORTED_LOCATION)
			install(FILES ${SDL2_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
		endif()
		if(PNG_FOUND AND NCINE_WITH_PNG)
			get_target_property(ZLIB_IMPORTED_LOCATION ZLIB::ZLIB IMPORTED_LOCATION)
			install(FILES ${ZLIB_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
			get_target_property(PNG_IMPORTED_LOCATION PNG::PNG IMPORTED_LOCATION)
			install(FILES ${PNG_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
		endif()
		if(WEBP_FOUND AND NCINE_WITH_WEBP)
			get_target_property(WEBP_IMPORTED_LOCATION WebP::WebP IMPORTED_LOCATION)
			install(FILES ${WEBP_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
		endif()
		if(OPENAL_FOUND AND NCINE_WITH_ADUIO)
			get_target_property(OPENAL_IMPORTED_LOCATION OpenAL::AL IMPORTED_LOCATION)
			install(FILES ${OPENAL_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})

			if(VORBIS_FOUND AND NCINE_WITH_VORBIS)
				get_target_property(OGG_IMPORTED_LOCATION Ogg::Ogg IMPORTED_LOCATION)
				install(FILES ${OGG_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
				get_target_property(VORBIS_IMPORTED_LOCATION Vorbis::Vorbis IMPORTED_LOCATION)
				install(FILES ${VORBIS_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
				get_target_property(VORBISFILE_IMPORTED_LOCATION Vorbis::Vorbisfile IMPORTED_LOCATION)
				install(FILES ${VORBISFILE_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
			endif()
		endif()
		if(LUA_FOUND AND NCINE_WITH_LUA)
			get_target_property(LUA_IMPORTED_LOCATION Lua::Lua IMPORTED_LOCATION)
			install(FILES ${LUA_IMPORTED_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
		endif()
	else()
		install(DIRECTORY ${MSVC_BINDIR}/ DESTINATION ${RUNTIME_INSTALL_DESTINATION} FILES_MATCHING PATTERN "*.dll")
	endif()
elseif(APPLE)
	if(NCINE_DYNAMIC_LIBRARY)
		file(RELATIVE_PATH RELPATH_TO_LIB ${CMAKE_INSTALL_PREFIX}/${RUNTIME_INSTALL_DESTINATION}/ ${CMAKE_INSTALL_PREFIX}/${LIBRARY_INSTALL_DESTINATION})
		install(FILES ${NCINE_LOCATION} DESTINATION ${LIBRARY_INSTALL_DESTINATION})
	else()
		# If the engine has been built statically the application needs to find the frameworks
		file(RELATIVE_PATH RELPATH_TO_LIB ${CMAKE_INSTALL_PREFIX}/${RUNTIME_INSTALL_DESTINATION}/ ${CMAKE_INSTALL_PREFIX}/${FRAMEWORKS_INSTALL_DESTINATION})
	endif()
	set_target_properties(${PACKAGE_EXE_NAME} PROPERTIES INSTALL_RPATH "@executable_path/${RELPATH_TO_LIB}")

	if(EXISTS ${NCINE_CONFIG_H})
		if(GLEW_FOUND AND NCINE_WITH_GLEW)
			install(DIRECTORY ${FRAMEWORKS_DIR}/glew.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
		endif()
		if(GLFW_FOUND AND NCINE_WITH_GLFW)
			install(DIRECTORY ${FRAMEWORKS_DIR}/glfw.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
		endif()
		if(SDL2_FOUND AND NCINE_WITH_SDL)
			install(DIRECTORY ${FRAMEWORKS_DIR}/sdl2.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
		endif()
		if(PNG_FOUND AND NCINE_WITH_PNG)
			install(DIRECTORY ${FRAMEWORKS_DIR}/zlib.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
			install(DIRECTORY ${FRAMEWORKS_DIR}/png.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
		endif()
		if(WEBP_FOUND AND NCINE_WITH_WEBP)
			install(DIRECTORY ${FRAMEWORKS_DIR}/webp.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
		endif()
		if(OPENAL_FOUND AND NCINE_WITH_AUDIO)
			install(DIRECTORY ${FRAMEWORKS_DIR}/openal.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})

			if(VORBIS_FOUND AND NCINE_WITH_VORBIS)
				install(DIRECTORY ${FRAMEWORKS_DIR}/ogg.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
				install(DIRECTORY ${FRAMEWORKS_DIR}/vorbis.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
				install(DIRECTORY ${FRAMEWORKS_DIR}/vorbisfile.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
			endif()
		endif()
		if(LUA_FOUND AND NCINE_WITH_LUA)
			install(DIRECTORY ${FRAMEWORKS_DIR}/lua.framework DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
		endif()
	else()
		install(DIRECTORY ${FRAMEWORKS_DIR}/ DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
	endif()
endif()