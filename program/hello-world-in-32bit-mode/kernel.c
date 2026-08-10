void main()
{
	char *video_memory	=	(char *)0xB8000;
	const char *message	=	"Hello, World!";
	
	for (int i = 0; i < 80 * 25 * 2; i = i + 2)
	{
		video_memory[i]		=	' ';
		video_memory[i + 1]	=	0x07;
	}
	
	int i	=	0;
	int j	=	0;
	while (message[i] != '\0')
	{
		video_memory[j]		=	message[i];
		video_memory[j + 1]	=	0x0B;
		
		i	=	i + 1;
		j	=	j + 2;
	}
	
	while (1)
	{}
}
