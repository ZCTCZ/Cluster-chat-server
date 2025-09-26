set(SRC_LIST
    main.cpp
    server.cpp
    service.cpp
    db/database.cpp
    model/usermodel.cpp
    model/offlinemessagemodel.cpp
    model/friendmodel.cpp
    model/groupmodel.cpp
    redis/redis.cpp)

add_executable(ClusterChatServer.out ${SRC_LIST})

target_include_directories(ClusterChatServer.out PUBLIC
                            ${PROJECT_SOURCE_DIR}/include
                            ${PROJECT_SOURCE_DIR}/include/server
                            ${PROJECT_SOURCE_DIR}/include/server/db
                            ${PROJECT_SOURCE_DIR}/include/server/model
                            ${PROJECT_SOURCE_DIR}/include/server/redis
                            ${PROJECT_BINARY_DIR}/include/client)

target_link_libraries(ClusterChatServer.out muduo_net muduo_base mysqlclient hiredis pthread)