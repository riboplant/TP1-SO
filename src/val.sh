valgrind --leak-check=full --show-reachable=yes --track-origins=yes --show-leak-kinds=all --track-fds=yes ./app
valgrind --leak-check=full --show-reachable=yes --track-origins=yes --show-leak-kinds=all --track-fds=yes ./slave
valgrind --leak-check=full --show-reachable=yes --track-origins=yes --show-leak-kinds=all --track-fds=yes ./view