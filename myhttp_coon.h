#pragma once
#ifndef _MYHTTP_COON_H
#define _MYHTTP_COON_H
#include<iostream>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<sys/sendfile.h>
#include<sys/epoll.h>
#include<sys/fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
using namespace std;
#define READ_BUF 2000
class http_coon {
public:
	/*NO_REQUESTION�Ǵ���������������Ҫ�ͻ��������룻BAD_REQUESTION��HTTP�����﷨����ȷ��GET_REQUESTION�����ò��ҽ�����һ����ȷ��HTTP����FORBIDDEN_REQUESTION�Ǵ��������Դ��Ȩ�������⣻FILE_REQUESTION����GET������Դ����INTERNAL_ERROR����������������⣻NOT_FOUND�����������Դ�ļ������ڣ�DYNAMIC_FILE��ʾ��һ����̬����POST_FILE��ʾ���һ����POST��ʽ�����HTTP����*/
	enum HTTP_CODE { NO_REQUESTION, GET_REQUESTION, BAD_REQUESTION, FORBIDDEN_REQUESTION, FILE_REQUESTION, INTERNAL_ERROR, NOT_FOUND, DYNAMIC_FILE, POST_FILE };
	/*HTTP���������״̬ת�ơ�HEAD��ʾ����ͷ����Ϣ��REQUESTION��ʾ����������*/
	enum CHECK_STATUS { HEAD, REQUESTION };
private:
	char requst_head_buf[1000];//��Ӧͷ�����
	char post_buf[1000];//Post����Ķ�������
	char read_buf[READ_BUF];//�ͻ��˵�http�����ȡ
	char filename[250];//�ļ���Ŀ¼
	int file_size;//�ļ���С
	int check_index;//Ŀǰ��⵽��λ��
	int read_buf_len;//��ȡ�������Ĵ�С
	char *method;//���󷽷�
	char *url;//�ļ�����
	char *version;//Э��汾
	char *argv;//��̬�������
	bool m_linger;//�Ƿ񱣳�����
	int m_http_count;//http����
	char *m_host;//��������¼
	char path_400[17];//������400�򿪵��ļ���������
	char path_403[23];//������403�򿪷��ص��ļ���������
	char path_404[40];//������404��Ӧ�ļ���������
	char message[1000];//��Ӧ��Ϣ�建����
	char body[2000];//post��Ӧ��Ϣ�建����
	CHECK_STATUS status;//״̬ת��
	bool m_flag;//true��ʾ�Ƕ�̬���󣬷�֮�Ǿ�̬����
public:
	int epfd;
	int client_fd;
	int read_count;
	http_coon();
	~http_coon();
	void init(int e_fd, int c_fd);//��ʼ��
	int myread();//��ȡ����
	bool mywrite();//��Ӧ����
	void doit();//�߳̽ӿں���
	void close_coon();//�رտͻ�������
private:
	HTTP_CODE analyse();//����Http����ͷ�ĺ���
	int jude_line(int &check_index, int &read_buf_len);//�������Ƿ�������������\r\n
	HTTP_CODE head_analyse(char *temp);//http����ͷ����
	HTTP_CODE requestion_analyse(char *temp);//http�����н���
	HTTP_CODE do_post();//��post�����еĲ������н���
	HTTP_CODE do_file();//��GET���󷽷��е�url Э��汾�ķ���
	void modfd(int epfd, int sock, int ev);//�ı�socketΪ״̬
	void dynamic(char *filename, char *argv);//ͨ��get��������Ķ�̬������
	void post_respond();//POST������Ӧ���
	bool bad_respond();//�﷨����������Ӧ���
	bool forbiden_respond();//��ԴȨ������������Ӧ�����
	bool succeessful_respond();//�����ɹ�������Ӧ���
	bool not_found_request();//��Դ������������Ӧ���
};

void http_coon::init(int e_fd, int c_fd)
{
	epfd = e_fd;
	client_fd = c_fd;
	read_count = 0;
	m_flag = false;
}

http_coon::http_coon()
{

}

http_coon::~http_coon()
{

}
/*�رտͻ�������*/
void http_coon::close_coon()
{
	epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, 0);
	close(client_fd);
	client_fd = -1;

}
/*�ı��¼����е��¼�����*/
void http_coon::modfd(int epfd, int client_fd, int ev)
{
	epoll_event event;
	event.data.fd = client_fd;
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
	epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &event);

}
/*read�����ķ�װ*/
int http_coon::myread()
{
	bzero(&read_buf, sizeof(read_buf));
	while (true)
	{
		int ret = recv(client_fd, read_buf + read_count, READ_BUF - read_count, 0);
		if (ret == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)//��ȡ����
			{
				break;
			}
			return 0;
		}
		else if (ret == 0)
		{
			return 0;
		}
		read_count = read_count + ret;
	}
	strcpy(post_buf, read_buf);
	return 1;
}
/*��Ӧ״̬����䣬���ﷵ�ؿ��Բ�Ϊbool����*/
bool http_coon::succeessful_respond()//200
{
	m_flag = false;
	bzero(requst_head_buf, sizeof(requst_head_buf));
	sprintf(requst_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}
bool http_coon::bad_respond()//400
{
	bzero(url, strlen(url));
	strcpy(path_400, "bad_respond.html");
	url = path_400;
	bzero(filename, sizeof(filename));
	sprintf(filename, "/home/jialuhu/linux_net/web_sever/%s", url);
	struct stat my_file;
	if (stat(filename, &my_file) < 0)
	{
		cout << "�ļ�������\n";
	}
	file_size = my_file.st_size;
	bzero(requst_head_buf, sizeof(requst_head_buf));
	sprintf(requst_head_buf, "HTTP/1.1 400 BAD_REQUESTION\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}
bool http_coon::forbiden_respond()//403
{
	bzero(url, strlen(url));
	strcpy(path_403, "forbidden_request.html");
	url = path_403;
	bzero(filename, sizeof(filename));
	sprintf(filename, "/home/jialuhu/linux_net/web_sever/%s", url);
	struct stat my_file;
	if (stat(filename, &my_file) < 0)
	{
		cout << "ʧ��\n";
	}
	file_size = my_file.st_size;
	bzero(requst_head_buf, sizeof(requst_head_buf));
	sprintf(requst_head_buf, "HTTP/1.1 403 FORBIDDEN\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}
bool http_coon::not_found_request()//404
{
	bzero(url, strlen(url));
	strcpy(path_404, "not_found_request.html");
	url = path_404;
	bzero(filename, sizeof(filename));
	sprintf(filename, "/home/jialuhu/linux_net/web_sever/%s", url);
	struct stat my_file;
	if (stat(filename, &my_file) < 0)
	{
		cout << "����\n";
	}
	file_size = my_file.st_size;
	bzero(requst_head_buf, sizeof(requst_head_buf));
	sprintf(requst_head_buf, "HTTP/1.1 404 NOT_FOUND\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

/*��̬������*/
void http_coon::dynamic(char *filename, char *argv)
{
	int len = strlen(argv);
	int k = 0;
	int number[2];
	int sum = 0;
	m_flag = true;
	bzero(requst_head_buf, sizeof(requst_head_buf));
	sscanf(argv, "a=%d&b=%d", &number[0], &number[1]);
	if (strcmp(filename, "/add") == 0)
	{
		sum = number[0] + number[1];
		sprintf(body, "<html><body>\r\n<p>%d + %d = %d </p><hr>\r\n</body></html>\r\n", number[0], number[1], sum);
		sprintf(requst_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length: %d\r\n\r\n", strlen(body));
	}
	else if (strcmp(filename, "/multiplication") == 0)
	{
		cout << "\t\t\t\tmultiplication\n\n";
		sum = number[0] * number[1];
		sprintf(body, "<html><body>\r\n<p>%d * %d = %d </p><hr>\r\n</body></html>\r\n", number[0], number[1], sum);
		sprintf(requst_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length: %d\r\n\r\n", strlen(body));
	}
}
/*POST������*/
void http_coon::post_respond()
{
	if (fork() == 0)
	{
		dup2(client_fd, STDOUT_FILENO);
		execl(filename, argv, NULL);
	}
	wait(NULL);
}

/*�ж�һ���Ƿ��ȡ����*/
int http_coon::jude_line(int &check_index, int &read_buf_len)
{
	cout << read_buf << endl;
	char ch;
	for (; check_index < read_buf_len; check_index++)
	{
		ch = read_buf[check_index];
		if (ch == '\r' && check_index + 1 < read_buf_len && read_buf[check_index + 1] == '\n')
		{
			read_buf[check_index++] = '\0';
			read_buf[check_index++] = '\0';
			return 1;//��������һ��
		}
		if (ch == '\r' && check_index + 1 == read_buf_len)
		{
			return 0;
		}
		if (ch == '\n')
		{
			if (check_index > 1 && read_buf[check_index - 1] == '\r')
			{
				read_buf[check_index - 1] = '\0';
				read_buf[check_index++] = '\0';
				return 1;
			}
			else {
				return 0;
			}
		}
	}
	return 0;
}

/*����������*/
http_coon::HTTP_CODE http_coon::requestion_analyse(char *temp)
{
	char *p = temp;
	cout << "p=" << p << endl;
	for (int i = 0; i < 2; i++)
	{
		if (i == 0)
		{
			method = p;//���󷽷�����
			int j = 0;
			while ((*p != ' ') && (*p != '\r'))
			{
				p++;
			}
			p[0] = '\0';
			p++;
			cout << "method:" << method << endl;
			//  method++;
		}
		if (i == 1)
		{
			url = p;//�ļ�·������
			while ((*p != ' ') && (*p != '\r'))
			{
				p++;
			}
			p[0] = '\0';
			p++;
			cout << "url:" << url << endl;
		}
	}
	version = p;//����Э�鱣��
	while (*p != '\r')
	{
		p++;
	}
	p[0] = '\0';
	p++;
	p[0] = '\0';
	p++;
	cout << version << endl;
	if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0)
	{
		return BAD_REQUESTION;
	}
	if (!url || url[0] != '/')
	{
		return BAD_REQUESTION;
	}
	if (strcmp(version, "HTTP/1.1") != 0)
	{
		return BAD_REQUESTION;
	}
	status = HEAD;//״̬ת�Ƶ�����ͷ��
	return NO_REQUESTION;//��������
}

/*����ͷ����Ϣ*/
http_coon::HTTP_CODE http_coon::head_analyse(char *temp)
{
	if (temp[0] == '\0')
	{
		//���һ������http����
		return GET_REQUESTION;
	}
	//��������ͷ��
	else if (strncasecmp(temp, "Connection:", 11) == 0)
	{
		temp = temp + 11;
		while (*temp == ' ')
		{
			temp++;
		}
		if (strcasecmp(temp, "keep-alive") == 0)
		{
			m_linger = true;
		}
	}
	else if (strncasecmp(temp, "Content-Length:", 15) == 0)
	{

		temp = temp + 15;
		while (*temp == ' ')
		{
			cout << *temp << endl;
			temp++;
		}
		m_http_count = atol(temp);//content-length��Ҫ���
	}
	else if (strncasecmp(temp, "Host:", 5) == 0)
	{
		temp = temp + 5;
		while (*temp == ' ')
		{
			temp++;
		}
		m_host = temp;
	}
	else {
		cout << "can't handle it's hand\n";
	}
	return NO_REQUESTION;
}

http_coon::HTTP_CODE http_coon::do_file()//GET�������󣬶��������н��н�������д��Դ·��
{
	char path[40] = "/home/jialuhu/linux_net/web_sever";
	char* ch;
	if (ch = strchr(url, '?'))
	{
		argv = ch + 1;
		*ch = '\0';
		strcpy(filename, url);
		return DYNAMIC_FILE;
	}
	else {
		strcpy(filename, path);
		strcat(filename, url);
		struct stat m_file_stat;
		if (stat(filename, &m_file_stat) < 0)
		{
			//cout << "�򲻿�\n";
			return NOT_FOUND;//NOT_FOUND 404
		}
		if (!(m_file_stat.st_mode & S_IROTH))//FORBIDDEN_REQUESTION 403
		{
			return FORBIDDEN_REQUESTION;
		}
		if (S_ISDIR(m_file_stat.st_mode))
		{
			return BAD_REQUESTION;//BAD_REQUESTION 400
		}
		file_size = m_file_stat.st_size;
		return FILE_REQUESTION;
	}
}
http_coon::HTTP_CODE http_coon::do_post()//POST�������󣬷ֽⲢ�Ҵ������
{
	int k = 0;
	int star;
	char path[34] = "/home/jialuhu/linux_net/web_sever";
	strcpy(filename, path);
	strcat(filename, url);
	star = read_buf_len - m_http_count;
	argv = post_buf + star;
	argv[strlen(argv) + 1] = '\0';
	if (filename != NULL && argv != NULL)
	{
		return POST_FILE;
	}
	return BAD_REQUESTION;
}

/*http�������*/
http_coon::HTTP_CODE http_coon::analyse()
{
	status = REQUESTION;
	int flag;
	char *temp = read_buf;
	int star_line = 0;
	check_index = 0;
	int star = 0;
	read_buf_len = strlen(read_buf);
	int len = read_buf_len;
	while ((flag = jude_line(check_index, len)) == 1)
	{
		temp = read_buf + star_line;
		star_line = check_index;
		switch (status)
		{
		case REQUESTION://�����з����������ļ����ƺ����󷽷�
		{
			cout << "requestion\n";
			int ret;
			ret = requestion_analyse(temp);
			if (ret == BAD_REQUESTION)
			{
				cout << "ret == BAD_REQUESTION\n";
				//�����ʽ����ȷ
				return BAD_REQUESTION;
			}
			break;
		}
		case HEAD://����ͷ�ķ���
		{
			int ret;
			ret = head_analyse(temp);
			if (ret == GET_REQUESTION)//��ȡ������HTTP����
			{
				if (strcmp(method, "GET") == 0)
				{
					return do_file();//GET�����ļ������뺯��     
				}
				else if (strcmp(method, "POST") == 0)
				{
					return do_post();//POST����������뺯��
				}
				else {
					return BAD_REQUESTION;
				}
			}
			break;
		}
		default:
		{
			return INTERNAL_ERROR;
		}
		}
	}
	return NO_REQUESTION;//������������Ҫ��������
}



/*�߳�ȡ����������Ľӿں���*/
void http_coon::doit()
{
	int choice = analyse();//���ݽ�������ͷ�Ľ����ѡ��
	switch (choice)
	{
	case NO_REQUESTION://��������
	{
		cout << "NO_REQUESTION\n";
		/*�ı�epoll������*/
		modfd(epfd, client_fd, EPOLLIN);
		return;
	}
	case BAD_REQUESTION: //400
	{
		cout << "BAD_REQUESTION\n";
		bad_respond();
		modfd(epfd, client_fd, EPOLLOUT);
		break;
	}
	case FORBIDDEN_REQUESTION://403
	{
		cout << "forbiden_respond\n";
		forbiden_respond();
		modfd(epfd, client_fd, EPOLLOUT);
		break;
	}
	case NOT_FOUND://404
	{
		cout << "not_found_request" << endl;
		not_found_request();
		modfd(epfd, client_fd, EPOLLOUT);
		break;
	}
	case FILE_REQUESTION://GET�ļ���Դ������
	{
		cout << "�ļ�file request\n";
		succeessful_respond();
		modfd(epfd, client_fd, EPOLLOUT);
		break;
	}
	case DYNAMIC_FILE://��̬������
	{
		cout << "��̬������\n";
		cout << filename << " " << argv << endl;
		dynamic(filename, argv);
		modfd(epfd, client_fd, EPOLLOUT);
		break;
	}
	case POST_FILE://POST ��������
	{
		cout << "post_respond\n";
		post_respond();
		break;
	}
	default:
	{
		close_coon();
	}

	}
}



bool http_coon::mywrite()
{
	if (m_flag)//����Ƕ�̬���󣬷��������
	{
		int ret = send(client_fd, requst_head_buf, strlen(requst_head_buf), 0);
		int r = send(client_fd, body, strlen(body), 0);
		if (ret > 0 && r > 0)
		{
			return true;
		}
	}
	else {
		int fd = open(filename, O_RDONLY);
		assert(fd != -1);
		int ret;
		ret = write(client_fd, requst_head_buf, strlen(requst_head_buf));
		if (ret < 0)
		{
			close(fd);
			return false;
		}
		ret = sendfile(client_fd, fd, NULL, file_size);
		if (ret < 0)
		{
			close(fd);
			return false;
		}
		close(fd);
		return true;
	}
	return false;
}
#endif
