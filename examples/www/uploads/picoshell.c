#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int picoshell(char **cmds[])
{
	int n_cmds = 0;
	int i = 0;
	
	while(cmds[n_cmds] != NULL)
		n_cmds ++;
	
	int pipes[n_cmds - 1][2];
	while (i < n_cmds - 1)
	{
		if (pipe(pipes[i]) == -1)
			return 1;
		i ++;
	}
	
	i = 0;
	while(i < n_cmds)
	{
		int pid = fork();
		if (pid == -1)
			exit(1);
		if (pid == 0)
		{
			if (i > 0)
			{
				dup2(pipes[i - 1][0], STDIN_FILENO);
				close(pipes[i - 1][1]);			
			}
			if (i < n_cmds - 1)
			{
				dup2(pipes[i][1], STDOUT_FILENO);
				close(pipes[i][0]);				
			}
			int j = 0;
			while(j < n_cmds - 1)
			{
				close(pipes[j][0]);
				close(pipes[j][1]);
				j ++;
			}
			execvp(cmds[i][0], cmds[i]);
			exit(1);			
		}
		else
		{
			if (i > 0)
			{ 
				close(pipes[i - 1][0]);
				close(pipes[i - 1][1]);
			}
		}
		i ++;
	}
	int j = 0;
	while(j < n_cmds - 1)
	{
		close(pipes[j][0]);
		close(pipes[j][1]);
		j ++;
	}
	j = 0;
	while(j < n_cmds)
	{
		wait(NULL);
		j ++;
	}
	return 0;
}
