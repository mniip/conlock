all: screen_recon screen_lock screen_dialog
	chmod +x screen_lock screen_dialog

suid: screen_recon
	chown root:root screen_recon
	chmod u+s screen_recon

screen_recon:
	cc -o screen_recon screen_recon.c

PREFIX=/usr/local
install: all
	cp -p screen_lock screen_dialog screen_recon $(PREFIX)/bin/
	chown root:root $(PREFIX)/bin/screen_recon
	chmod u+s $(PREFIX)/bin/screen_recon

uninstall:
	rm -f $(PREFIX)/bin/{screen_lock,screen_dialog,screen_recon}

.PHONY: all suid install uninstall
