#include "dmxconsole.h"

int main(int argc, char *argv[])
{	
	DMXConsole console(argv, argc);
	console.extract();

	return 0;
}
