linux:
	cd lua && ($(MAKE) linux || /bin/echo -e "\033[41;37mfailure: install libreadline-dev first\033[0m") && cd ..
	cd lpeg && $(MAKE) LUADIR="../lua/src/" linux && cd ..

install:
	cd lua && $(MAKE) install && cd ..
	@echo `lua/src/lua -e "print(string.match(package.cpath, '[^?;]+'))"`
	cp lpeg/lpeg.so `lua/src/lua -e "print(string.match(package.cpath, '[^?;]+'))"`

test:
	cd lua && $(MAKE) test && cd ..
	cd lpeg && $(MAKE) test && cd ..

clean:
	cd lua && $(MAKE) clean && cd ..
	cd lpeg && $(MAKE) clean && cd ..

uninstall:
	@echo `lua -e "print(string.match(package.cpath, '[^?;]+'))"`lpeg.so
	rm -f `lua -e "print(string.match(package.cpath, '[^?;]+'))"`lpeg.so
	cd lua && $(MAKE) uninstall && cd ..
