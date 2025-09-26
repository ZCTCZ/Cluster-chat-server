add_executable(ClusterChatClient.out main.cpp client.cpp)

target_link_libraries(ClusterChatClient.out pthread)