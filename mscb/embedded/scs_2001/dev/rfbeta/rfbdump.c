#include <stdio.h>
#include <mscb.h>
#include <time.h>
#include <string.h>

void main()
{
	int i, fd, size;
	float t[8], v[8];
	unsigned char state;
	char str[256], filename[80], line[256];
	time_t now;
	FILE *f;

	fd = mscb_init("192.168.1.36", 0, NULL, 0);
    if (fd < 0) {
		printf("Cannot connect to MSCB submaster. Please check network.\n");
		return;
	}
	time(&now);
	strcpy(str, ctime(&now));
	sprintf(filename, "rfbdump_%c%c_%c%c_%c%c-%c%c_%c%c_%c%c.txt", str[8], str[9], '0', '4', '1', '2', str[11], str[12], str[14], str[15], str[17], str[18]);
	f = fopen(filename, "wt");
	printf("Writing to %s\n\n", filename);
	do {
		size = sizeof(t[0]);
		for (i=0 ; i<8 ; i++)
			mscb_read(fd, 1, i, &t[i], &size);

		for (i=0 ; i<8 ; i++)
			mscb_read(fd, 1, 8+i, &v[i], &size);
	
		size = sizeof(state);
		mscb_read(fd, 1, 17, &state, &size);

		time(&now);
		strcpy(str, ctime(&now));
		str[19] = 0;
		sprintf(line, "%s %d %1.2lf %1.1lf %1.1lf %1.1lf %1.1lf %1.1lf %1.1lf %1.1lf %1.1lf\n", str, state, v[0], t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7]);
		printf(line);
		fprintf(f, line);
		_sleep(1000);
	} while (!kbhit());

	fclose(f);
}