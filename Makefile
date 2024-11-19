install: pomodoro.c
	mkdir ~/.config/simple-pomodoro-cli
	cp -r audio ~/.config/simple-pomodoro-cli/
	gcc -lncurses -lm pomodoro.c src/config.c -o simple-pomodoro-cli
	mv simple-pomodoro-cli /bin/
