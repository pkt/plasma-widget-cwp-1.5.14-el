cmake_minimum_required(VERSION 2.6)

project(plasma-cwp)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_BINARY_DIR}
	${KDE4_INCLUDES}
	${QT_QTTEST_INCLUDE_DIR}
	)

set(cwp_SRCS plasma-cwp-graphics-widget.cpp plasma-cwp-pixmap-dialog.cpp plasma-cwp-pixmap-list-dialog.cpp plasma-cwp-data-provider.cpp plasma-cwp-paint-helper.cpp plasma-cwp-popup-applet.cpp plasma-cwp.cpp config.cpp)

add_subdirectory(po)

kde4_add_ui_files(cwp_SRCS config.ui)
kde4_add_plugin(plasma_applet_cwp ${cwp_SRCS})
target_link_libraries(plasma_applet_cwp
	${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY})

install(TARGETS
	plasma_applet_cwp
	DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES
	plasma-applet-cwp.desktop
	DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES
	images/background_circle_white.png
	images/background_rectangle_white.png
	images/background_rounded_rectangle_white.png
	images/background_circle_black.png
	images/background_rectangle_black.png
	images/background_rounded_rectangle_black.png
	images/background_circle_grey.png
	images/background_rectangle_grey.png
	images/background_rounded_rectangle_grey.png
	data/br_weather_com.xml
	data/de_weather_com.xml
	data/espanol_weather_com.xml
	data/fr_weather_com.xml
	data/in_weather_com.xml
	data/uk_weather_com.xml
	data/www_weather_com.xml
	data/wetter_com.xml
	data/www_weather_com.cn.xml
	data/myforecast_com_celsius.xml
	data/myforecast_com_fahrenheit.xml
	data/www_accuweather_com_world_celsius.xml
	data/www_accuweather_com_world_fahrenheit.xml
	data/www_accuweather_com_us_celsius.xml
	data/www_accuweather_com_us_fahrenheit.xml
	data/ilmeteo_it.xml
	data/pogodynka_pl.xml
	data/www_gismeteo_com.xml
	data/freemeteo_com_celsius.xml
	data/freemeteo_com_fahrenheit.xml
	data/satellite_images.xml
	DESTINATION ${DATA_INSTALL_DIR}/plasma-cwp)

install(FILES
	images/wind_arrow.svgz
	images/wind_unknown.svgz
	images/wind_var.svgz
	images/satellite_image.svgz
	DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/)

install(FILES
	images/weather-windy.svgz
	DESTINATION ${SHARE_INSTALL_PREFIX}/icons/oxygen/scalable/status/)

install(FILES
	images/weather-windy.png
	DESTINATION ${SHARE_INSTALL_PREFIX}/icons/oxygen/128x128/status/)
