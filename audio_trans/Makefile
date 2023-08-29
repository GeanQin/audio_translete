all:
	gcc test.c aac_trans.c g711a_trans.c opus_trans.c -I../thirdparty/include -I./ -L../thirdparty/lib -lfaac -lm -lfaad -lopus -o audio_trans

clean:
	rm -rf audio_trans out.*
