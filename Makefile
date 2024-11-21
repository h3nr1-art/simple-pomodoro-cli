install: pomodoro.c
	gcc -lncurses -lm pomodoro.c src/config.c -o simple-pomodoro-cli
	mv simple-pomodoro-cli /bin/
