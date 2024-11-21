<<<<<<< HEAD
install: pomodoro.c
	gcc -lncurses -lm pomodoro.c src/config.c -o simple-pomodoro-cli
=======
prepare:
	mkdir $(HOME)/.config/simple-pomodoro-cli
	cp -rf ./audio/ $(HOME)/.config/simple-pomodoro-cli/
	gcc -lncurses -lm pomodoro.c src/config.c -o simple-pomodoro-cli

install: pomodoro.c
>>>>>>> 73f9f18 (updated make file)
	mv simple-pomodoro-cli /bin/
