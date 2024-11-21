prepare:
	mkdir $(HOME)/.config/simple-pomodoro-cli
	cp -rf ./audio/ $(HOME)/.config/simple-pomodoro-cli/
	gcc -lncurses -lm pomodoro.c src/config.c -o simple-pomodoro-cli

install: pomodoro.c
	mv simple-pomodoro-cli /bin/
