all:
	make -C src

clean:
	make -C src clean
	make -C bin clean
	rm -fv *~ */*~

save:
	bin/backup
	cp player/* reimb
	sync

local-distclean:
	-rm area/*
	-rm clans/*
	-rm deleted/area/*
	-rm deleted/player/*
	-rm files/*
	-rm mail/*
	-rm notes/* notes/clans/*
	-rm player/*
	-rm prev/area/*
	-rm prev/files/*
	-rm prev/player/*
	-rm prev/rooms/*
	-rm prev/tables/*
	-rm rooms/*
	-rm tables/*
	-rm logs/mob/*
	-rm logs/object/*
	-rm logs/player/*
	-rm logs/room/*
