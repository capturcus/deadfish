set(PACKAGE_NAME "DeadFishClient")
set(PACKAGE_VENDOR "DeadFish Team")
set(PACKAGE_COPYRIGHT "Copyright ©2020 ${PACKAGE_VENDOR}")
set(PACKAGE_DESCRIPTION "The WebAssembly client for DeadFish")
set(PACKAGE_HOMEPAGE "https://capturcus.github.io/deadfish/")
set(PACKAGE_REVERSE_DNS "io.github.capturcus.deadfish")

set(PACKAGE_INCLUDE_DIRS include)

file(GLOB SourceFiles src/*.cpp)

set(PACKAGE_SOURCES
	${SourceFiles}
)
