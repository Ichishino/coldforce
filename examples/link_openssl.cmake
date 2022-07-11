find_package(OpenSSL QUIET)

if (OPENSSL_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
elseif (APPLE)
    target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/opt/openssl/include)
    link_directories(${PROJECT_NAME} /usr/local/opt/openssl/lib)
    target_link_libraries(${PROJECT_NAME} -lssl -lcrypto)
endif()

