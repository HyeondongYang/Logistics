#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 30
#define STOCK_SIZE 300

typedef struct product_data
{
    char name[NAME_SIZE];
    int stock;
} prod_data;

prod_data product_stock[30] = {
    {"우스터소스", 0},
    {"마요네즈", 0},
    {"토마토케첩", 0},
    {"핫소스", 0},
    {"칠리소스", 0},
    {"올리브유", 0},
    {"허니머스터드", 0},
    {"홀스래디시", 0},
    {"바질패스토", 0},
    {"고추장", 0},
    {"된장", 0},
    {"간장", 0},
    {"데리야키소스", 0},
    {"아몬드소스", 0},
    {"트러플소스", 0},
    {"타바스코소스", 0},
    {"레드와인소스", 0},
    {"돈까스소스", 0},
    {"타르타르소스", 0},
    {"카르보나라소스", 0},
    {"초콜릿소스", 0},
    {"오렌지소스", 0},
    {"망고소스", 0},
    {"후추소스", 0},
    {"버터소스", 0},
    {"발사믹드레싱", 0},
    {"바비큐소스", 0},
    {"렌치소스", 0},
    {"과카몰리소스", 0},
    {"마라소스", 0}};

int chat_mode = 0;

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);
//void show_list();

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

void show_list_visual()
{
    for (int k = 0; k <= 24; k += 6)
    {
        for (int i = k; i < k + 6; i++)
            printf("┌──────────────────┐");
        printf("\n");
        for (int i = k; i < k + 6; i++)
        {
            printf("│");
            for (int j = 0; j < 9 - strlen(product_stock[i].name) / 3; j++)
                printf(" ");
            printf("%s",product_stock[i].name);
            for (int j = 0; j < 9 - strlen(product_stock[i].name) / 3; j++)
                printf(" ");
            printf("│");
        }
        printf("\n");
        for (int i = k; i < k + 6; i++)
            printf("│      %4d개      │", product_stock[i].stock);
        printf("\n");
        for (int i = k; i < k + 6; i++)
            printf("└──────────────────┘");
        printf("\n");
        
    }
}


int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;

    system("clear");

    if (argc != 4)
    {
        printf("Usage: %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);
    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

   

    while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        printf("connect() error!\nretry...\n");
        sleep(1);
    }
    system("clear");    
    show_list_visual();

    write(sock, name, NAME_SIZE); // 아이디 서버로 보냄
    // read(sock, product_stock, sizeof(product_stock));
    // printf("%s\n", product_stock->name);
    // fputs(product_stock,stdout);

    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock); //통신은 이 쓰레드 두개가 처리함 메인은 대기
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    close(sock);

    return 0;
}

/*void show_list() // 재고 출력함수------------
{
    sleep(1);
    system("clear");
    printf("==창고 재고==\n");
    for (int i = 0; i < 30; i++)
    {
        printf("%s : %d  ", product_stock[i].name, product_stock[i].stock);
        if (i % 5 == 4) // 5개씩 나눠서 출력!!
        {
            printf("\n");
        }
    }
}*/

void *send_msg(void *arg)
{
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    char send_buf[BUF_SIZE];
    char stock_input[50];
    int num_input;
    int i = 0;
    int str_len;
    while (1)
    {
        fgets(msg,sizeof(msg), stdin);
        if (chat_mode == 1)
        {
            sprintf(name_msg, "%s %s", name, msg);   // name,msg 입력받은걸 한문장으로 엮어서 name_msg에 저장
            write(sock, name_msg, strlen(name_msg)); //모드1 일떄 서버로 메세지 보내기
        }
        if(chat_mode==2)
        {
            sprintf(name_msg, "%s %s", name, msg);   
            write(sock, name_msg, strlen(name_msg)); //모드 2일때 서버로 메세지 보내기
        }

        else if (strstr(msg, "배송요청") != NULL)
        {
            printf("소스 입력: ");
            scanf("%s", stock_input);
            for (i = 0; i <= 30; i++)
            {
                if (strstr(product_stock[i].name, stock_input) != NULL)
                {
                    printf("개수 입력: ");
                    scanf("%d", &num_input);
                    sprintf(send_buf, "%s %d", product_stock[i].name, num_input);
                    write(sock, "배송요청",strlen("배송요청"));//"배송요청"메세지 서버로 보내기
                    sleep(1);
                    write(sock, send_buf,strlen(send_buf));//소스이름,개수 서버로 보내기
                    
                    str_len = read(sock, name_msg, BUF_SIZE - 1); 
                    if (str_len == -1)
                        printf("sss\n");

                    printf("왔냐? %s \n", name_msg);
                    if(strstr(name_msg, "싫어")!=NULL){
                        printf("야 싫다는데?\n");
                        break;
                    }
                    product_stock[i].stock += atoi(name_msg);
                    printf("바뀜?> %d\n", product_stock[i].stock);
                    show_list_visual();
                    break;
                }
            }
        }
        else if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
        {
            close(sock);
            exit(0);
        } // sprintf(name_msg, "%s %s", name, msg);   // name,msg 입력받은걸 한문장으로 엮어서 name_msg에 저장
        // else if (strstr(msg, "채팅종료") != NULL)
        //{
        //     sprintf(name_msg, "%s %s", name, msg);   // name,msg 입력받은걸 한문장으로 엮어서 name_msg에 저장
        //     write(sock, name_msg, strlen(name_msg)); //서버로 메세지 보내기
        // }
    }

    return NULL;
}

void *recv_msg(void *arg)
{
    int sock = *((int *)arg); //통신용 소켓 받음
    char name_msg[BUF_SIZE];
    char *temp_stock;
    char *temp_num;
    prod_data stock;
    int str_len;

    while (1)
    {   
        memset(&name_msg, 0, BUF_SIZE);
        str_len = read(sock, name_msg, BUF_SIZE - 1);
        if (str_len == -1)
            return (void *)-1;
        if (chat_mode == 1||chat_mode==2)
        {
            if (strstr(name_msg, "채팅종료") != NULL)
            {
                printf("채팅끝낸다잉~\n");
                chat_mode = 0;
            }else{
                printf("%s", name_msg);
                //fputs(name_msg, stdout);
            }
            
        }
        else if (strstr(name_msg, "지점배송") != NULL)
        {
            str_len = read(sock, name_msg, BUF_SIZE - 1); //"지점배송"을 읽으면 그때부터 메세지 읽음 

            name_msg[str_len] = 0;

            temp_stock = strtok(name_msg, "s");
            temp_stock = strtok(temp_stock, " ");
            temp_num = strtok(NULL, " "); //자른 문자 다음부터 구분자 또 찾기

            // strcpy(stock.name, temp_stock);

            for (int i = 0; i <= 30; i++)
            {
                if (strstr(temp_stock, product_stock[i].name) != NULL)
                {
                    product_stock[i].stock += atoi(temp_num);
                    printf("이름: %s \n", product_stock[i].name);
                    printf("숫자: %d\n", product_stock[i].stock);
                    printf("%d개 추가됨 \n", product_stock[i].stock);
                    show_list_visual();
                    break;
                }
            }
        }
        else if (strstr(name_msg, "창고반품") != NULL)
        {
            str_len = read(sock, name_msg, BUF_SIZE - 1); //"창고반품"읽으면 그떄 읽음 
            
            name_msg[str_len] = 0;

            //temp_stock = strtok(name_msg, "%");
            temp_stock = strtok(temp_stock, " ");
            temp_num = strtok(NULL, " "); //자른 문자 다음부터 구분자 또 찾기

            // strcpy(stock.name, temp_stock);

            for (int i = 0; i <= 30; i++)
            {
                if (strstr(temp_stock, product_stock[i].name) != NULL)
                {
                    product_stock[i].stock -= atoi(temp_num);
                    printf("이름: %s \n", product_stock[i].name);
                    printf("숫자: %d\n", product_stock[i].stock);
                    printf("%d개 추가됨 \n", product_stock[i].stock);
                    show_list_visual();
                }
            }
        }
        else if (strstr(name_msg, "채팅시작") != NULL)
        {
            printf("채팅한다잉~\n");
            chat_mode = 1;
        }
        else if(strstr(name_msg,"일대일채팅")!=NULL)
        {
            printf("일대일 시작~!\n");
            memset(&name_msg, 0, BUF_SIZE);
            chat_mode=2;
        }
    }

    return NULL;
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}