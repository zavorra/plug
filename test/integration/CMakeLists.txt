
add_executable(CommIT Main.cpp)
target_link_libraries(CommIT
                        PRIVATE
                            plug-mustang
                            plug-version
                            plug-communication
                            libusb-1.0::libusb-1.0
                            build-libs
                            )

