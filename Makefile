router: router.c 
	g++ -pthread -o router router.c  

clean: 
	rm router .route.dat
