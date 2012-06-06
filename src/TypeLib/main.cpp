//
//  main.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <stdio.h>
#include <crtdbg.h>

extern "C"
int Parse(char *file);
void Dump(FILE *fp);
void Dump2(FILE *fp);
void Cleanup();

int main(int argc, char *argv[])
{

	if(Parse("test.txt"))
	{
		Dump2(stdout);

		Dump(stdout);
	}

	Cleanup();

	return 0;
}

