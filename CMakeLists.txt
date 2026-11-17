cmake_minimum_required(VERSION 3.16)

project(PdfBookmarkViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Widgets Pdf PdfWidgets)

qt_standard_project_setup()

qt_add_executable(PdfBookmarkViewer
    src/main.cpp
    src/main_window.h
    src/main_window.cpp
    src/bookmark_store.h
    src/bookmark_store.cpp
)

target_link_libraries(PdfBookmarkViewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Pdf
    Qt6::PdfWidgets
)

set_target_properties(PdfBookmarkViewer PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# На Windows программа запускается прямо из папки сборки, поэтому рядом с
# исполняемым файлом должны лежать библиотеки Qt. Копируем их из того же
# комплекта Qt, которым собирали проект: библиотеки разных версий Qt
# несовместимы и дают ошибку «точка входа в процедуру не найдена».
if(WIN32)
    set(_qt_bin_dir "")
    if(QT6_INSTALL_PREFIX AND QT6_INSTALL_BINS)
        if(IS_ABSOLUTE "${QT6_INSTALL_BINS}")
            set(_qt_bin_dir "${QT6_INSTALL_BINS}")
        else()
            set(_qt_bin_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
        endif()
    endif()
    if(NOT _qt_bin_dir)
        get_target_property(_qt_core_location Qt6::Core LOCATION)
        if(_qt_core_location)
            get_filename_component(_qt_bin_dir "${_qt_core_location}" DIRECTORY)
        endif()
    endif()

    if(EXISTS "${_qt_bin_dir}/Qt6Core.dll")
        find_program(WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt
            HINTS "${_qt_bin_dir}"
            NO_DEFAULT_PATH)
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET PdfBookmarkViewer POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --force
                    --no-compiler-runtime
                    "$<TARGET_FILE:PdfBookmarkViewer>"
                VERBATIM)
        else()
            add_custom_command(TARGET PdfBookmarkViewer POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${_qt_bin_dir}/Qt6Core.dll"
                    "${_qt_bin_dir}/Qt6Gui.dll"
                    "${_qt_bin_dir}/Qt6Widgets.dll"
                    "${_qt_bin_dir}/Qt6Pdf.dll"
                    "${_qt_bin_dir}/Qt6PdfWidgets.dll"
                    "$<TARGET_FILE_DIR:PdfBookmarkViewer>"
                VERBATIM)
            set(_qt_plugin_platform "${QT6_INSTALL_PLUGINS}/platforms/qwindows.dll")
            if(EXISTS "${_qt_plugin_platform}")
                add_custom_command(TARGET PdfBookmarkViewer POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E make_directory
                        "$<TARGET_FILE_DIR:PdfBookmarkViewer>/platforms"
                    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "${_qt_plugin_platform}"
                        "$<TARGET_FILE_DIR:PdfBookmarkViewer>/platforms/"
                    VERBATIM)
            endif()
        endif()
    endif()
endif()
