#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio_ext.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 30
#define EPOLL_SIZE 50
#define STOCK_SIZE 300
#define WAIT_TIME 3




typedef struct product_data
{
    char name[NAME_SIZE]; // 물품 이름
    int stock; // 물품 개수
} prod_data; // 물품 데이터

typedef struct user_data
{
    char name[NAME_SIZE]; // 클라이언트(지점)의 이름

} user_data;

user_data user_data_list[EPOLL_SIZE]; // 유저 데이터 저장 배열

prod_data MV_Product_list[EPOLL_SIZE]; // 배송중 물품 저장 배열

prod_data BD_product_list[EPOLL_SIZE][30]; // 지점의 창고 현황 관리 배열

prod_data HD_product_stock[30] = {
    {"우스터소스", STOCK_SIZE},
    {"마요네즈", STOCK_SIZE},
    {"토마토케첩", STOCK_SIZE},
    {"핫소스", STOCK_SIZE},
    {"칠리소스", STOCK_SIZE},
    {"올리브유", STOCK_SIZE},
    {"허니머스터드", STOCK_SIZE},
    {"홀스래디시", STOCK_SIZE},
    {"바질패스토", STOCK_SIZE},
    {"고추장", STOCK_SIZE},
    {"된장", STOCK_SIZE},
    {"간장", STOCK_SIZE},
    {"데리야키소스", STOCK_SIZE},
    {"아몬드소스", STOCK_SIZE},
    {"트러플소스", STOCK_SIZE},
    {"타바스코소스", STOCK_SIZE},
    {"레드와인소스", STOCK_SIZE},
    {"돈까스소스", STOCK_SIZE},
    {"타르타르소스", STOCK_SIZE},
    {"카르보나라소스", STOCK_SIZE},
    {"초콜릿소스", STOCK_SIZE},
    {"오렌지소스", STOCK_SIZE},
    {"망고소스", STOCK_SIZE},
    {"후추소스", STOCK_SIZE},
    {"버터소스", STOCK_SIZE},
    {"발사믹드레싱", STOCK_SIZE},
    {"바비큐소스", STOCK_SIZE},
    {"렌치소스", STOCK_SIZE},
    {"과카몰리소스", STOCK_SIZE},
    {"마라소스", STOCK_SIZE}};

pthread_mutex_t mutex; // 다른 스레드가 같은 데이터에 동시에 접근하지 못하게 하기위해 mutex 사용

// epoll용 변수 선언
struct epoll_event *ep_events;
struct epoll_event event;
int epfd, event_cnt;
int chat_mode = 0;

char buf[BUF_SIZE];

int clnt_sock_list[EPOLL_SIZE]; // 채팅에 참가자 소켓번호 목록
int clnt_num = 0;               // 채팅 참가자 수

int roop_ctrl = 1; // 무한 루프 제어용 전역변수

pthread_t t_id_list[EPOLL_SIZE]; // 스레드 생성을 위한 변수
int t_id_cnt = 0; // 총 스레드 카운트용 변수
int MV_cnt=0;     // 배송중인 물품 배열 용 변수

void error_handling(char *message) // 에러메세지 출력
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE); // -1
}

void show_list_visual() // 재고 출력함수 2
{
    int i, j, k, n;
    printf("==========창고 물품==========\n\n");
    for (k = 0; k <= 24; k += 6)
    {
        for (i = k; i < k + 6; i++)
            printf("┌──────────────────┐");
        printf("\n");
        for (i = k; i < k + 6; i++)
        {
            printf("│");
            for (j = 0; j < 9 - strlen(HD_product_stock[i].name) / 3; j++)
                printf(" ");
            printf("%s",HD_product_stock[i].name);
            for (j = 0; j < 9 - strlen(HD_product_stock[i].name) / 3; j++)
                printf(" ");
            printf("│");
        }
        printf("\n");
        for (i = k; i < k + 6; i++)
            printf("│      %4d개      │", HD_product_stock[i].stock);
        printf("\n");
        for (i = k; i < k + 6; i++)
            printf("└──────────────────┘");
        printf("\n");        
    }
    for(n = 0; n < clnt_num; n++){
        printf("\n\n==========%s지점 물품==========\n\n", user_data_list[n].name);
        for (k = 0; k <= 24; k += 6)
        {
            for (i = k; i < k + 6; i++)
                printf("┌──────────────────┐");
            printf("\n");
            for (i = k; i < k + 6; i++)
            {
                printf("│");
                for (j = 0; j < 9 - strlen(BD_product_list[n][i].name) / 3; j++)
                    printf(" ");
                printf("%s",BD_product_list[n][i].name);
                for (j = 0; j < 9 - strlen(BD_product_list[n][i].name) / 3; j++)
                    printf(" ");
                printf("│");
            }
            printf("\n");
            for (i = k; i < k + 6; i++)
                printf("│      %4d개      │", BD_product_list[n][i].stock);
            printf("\n");
            for (i = k; i < k + 6; i++)
                printf("└──────────────────┘");
            printf("\n");            
        }
    }
    for(n=0; n<MV_cnt; n++){
        if(n==0)
            printf("\n\n==========배송 중==========\n");
        for (k = 0; k <= 24; k += 6)
        {
            for (i = k; i < k + 6; i++)
                printf("┌──────────────────┐");
            printf("\n");
            for (i = k; i < k + 6; i++)
            {
                printf("│");
                for (j = 0; j < 9 - strlen(MV_Product_list[i].name) / 3; j++)
                    printf(" ");
                printf("%s", MV_Product_list[i].name);
                for (j = 0; j < 9 - strlen(MV_Product_list[i].name) / 3; j++)
                    printf(" ");
                printf("│");
            }
            printf("\n");
            for (i = k; i < k + 6; i++){
                if(MV_Product_list[i].stock != 0)
                    printf("│      %4d개      │", MV_Product_list[i].stock);
                else
                    printf("│                  │");
            }
                
            printf("\n");
            for (i = k; i < k + 6; i++)
                printf("└──────────────────┘");
            printf("\n");        
        }
    }
    printf("\n\n[지점배송]\t[창고반품]\n[채팅시작]\t[일대일채팅]\t[채팅종료]\n");
}

void create_new_prod_list(int sock_num)
{
    int i;
    for (i = 0; i < 31; i++)
    {
        strcpy(BD_product_list[sock_num][i].name, HD_product_stock[i].name);
        BD_product_list[sock_num][i].stock = 0;
    }
    show_list_visual();
    return;
}

void remove_Client(int sock_num)
{
    int i;

    // 폴링 해제, 클라이언트 소켓 닫기
    epoll_ctl(epfd, EPOLL_CTL_DEL, sock_num, NULL);
    close(sock_num);

    // 제거되고 뒤에 있는 것을 앞으로 당기는 작업
    for (i = 0; i < clnt_num; i++)
    {
        if (clnt_sock_list[i] == sock_num)
        {
            
            memcpy(&clnt_sock_list[i], &clnt_sock_list[i + 1], sizeof(int) * (clnt_num - (i + 1))); // 소켓 리스트를 한칸씩 앞으로 당겨서 지워진 부분 매꾸기
            break;
        }
    }
    clnt_num--;
    printf("현재 참가자 수 = %d\n", clnt_num);
    show_list_visual();
}

void* clnt_to_serv(void* arg){
    int fd = *((int*)arg);
    int str_len;
    char name_msg[BUF_SIZE];
    char *temp_stock;
    char *temp_num;
    char user_input[BUF_SIZE];
    int i = 0, j ,k;
    int cur;

    str_len = read(fd, name_msg, BUF_SIZE - 1); //배송요청 했을시에만 read----------------------------------
    name_msg[str_len] = 0;
    temp_stock = strtok(name_msg, "s");
    temp_stock = strtok(temp_stock, " ");
    temp_num = strtok(NULL, " "); //자른 문자 다음부터 구분자 또 찾기
    //printf("fd: %d\n", fd);
    printf("이름: %s \n", temp_stock);
    printf("숫자: %s\n", temp_num);
    for (int i = 0; i <= 30; i++)
    {
        if (strstr(HD_product_stock[i].name, temp_stock) != NULL)
        {
            printf("요청옴 수락? y/n>> ");
            scanf("%s", user_input);
            if (strstr(user_input, "y") != NULL || strstr(user_input, "Y") != NULL)
            {
                //printf("e야 들어와라 \n");
                pthread_mutex_lock(&mutex);
                HD_product_stock[i].stock -= atoi(temp_num);
                strcpy(MV_Product_list[MV_cnt].name, HD_product_stock[i].name);
                MV_Product_list[MV_cnt].stock = atoi(temp_num);
                cur = MV_cnt++;
                pthread_mutex_unlock(&mutex);

                show_list_visual();
                sleep(WAIT_TIME);
                printf("변함? %d \n", HD_product_stock[i].stock);
                write(fd, temp_num,sizeof(temp_num));
                pthread_mutex_lock(&mutex);
                for(k=0; k < MV_cnt; k++){
                    if(k==cur){
                        memcpy(&MV_Product_list[k], &MV_Product_list[k + 1], sizeof(prod_data) * (MV_cnt - (k + 1))); // 소켓 리스트를 한칸씩 앞으로 당겨서 지워진 부분 매꾸기
                        break;
                    }
                }
                MV_cnt--;
                pthread_mutex_unlock(&mutex);
                for(j=0; j<clnt_num; j++){
                    if(clnt_sock_list[j] == fd){
                        BD_product_list[j][i].stock += atoi(temp_num);
                    }
                }
                

                show_list_visual();

            }
            else
            {
                write(fd, "싫어",strlen("싫어")); //"싫어" 보내기 
            }
            break;
        }
    }
    
    return (void*) 0;
}

void* serv_to_clnt(void* arg){
    char* user_input = (char*)arg;
    char stock_input[50];
    char name_input[50];
    char send_buf[BUF_SIZE];
    int num_input;
    int i, j, k;
    int cur;
    if (strstr(user_input, "지점배송") != NULL)
    {
        printf("소스 입력: ");
        scanf("%s", stock_input);
        for (i = 0; i <= 30; i++)
        {
            if (strstr(HD_product_stock[i].name, stock_input) != NULL)
            {
                printf("개수 입력: ");
                scanf("%d", &num_input);
                sprintf(send_buf, "%s %d", HD_product_stock[i].name, num_input);
                break;
            }
        }
        if(i>=31){
            printf("뭐라는겨?\n");
            return (void*) -1;
        }
        printf("지점 이름: ");
        scanf("%s", name_input);
        for (j = 0; j < clnt_num; j++)
        {
            if (strstr(user_data_list[j].name, name_input) != NULL){
                pthread_mutex_lock(&mutex);
                HD_product_stock[i].stock -= num_input;
                strcpy(MV_Product_list[MV_cnt].name, HD_product_stock[i].name);
                MV_Product_list[MV_cnt].stock = num_input;
                cur = MV_cnt++;
                pthread_mutex_unlock(&mutex);

                show_list_visual();
                sleep(WAIT_TIME);
                write(clnt_sock_list[j], "지점배송",strlen("지점배송")); //"지점배송"메세지 보내기
                sleep(1);
                write(clnt_sock_list[j], send_buf,strlen(send_buf)); //소스이름+개수+지점 합친 메세지 보내기 
                pthread_mutex_lock(&mutex);
                for(k=0; k < MV_cnt; k++){
                    if(k==cur){
                        memcpy(&MV_Product_list[k], &MV_Product_list[k + 1], sizeof(prod_data) * (MV_cnt - (k + 1))); // 소켓 리스트를 한칸씩 앞으로 당겨서 지워진 부분 매꾸기
                        break;
                    }
                }
                MV_cnt--;
                pthread_mutex_unlock(&mutex);
                BD_product_list[j][i].stock += num_input;

                show_list_visual();
                break;
            }
                
        }
        if(j>=clnt_num){
            printf("뭐라는겨?\n");
            return (void*) -1;
        }
        
        
    }
    else if (strstr(user_input, "창고반품") != NULL)
    {
        printf("소스 입력: ");
        scanf("%s", stock_input);
        for (i = 0; i <= 30; i++)
        {
            if (strstr(HD_product_stock[i].name, stock_input) != NULL)
            {
                printf("개수 입력: ");
                scanf("%d", &num_input);
                sprintf(send_buf, "%s %d", HD_product_stock[i].name, num_input);
                break;
            }
        }
        if(i>=31){
            printf("뭐라는겨?\n");
            return (void*) -1;
        }
        printf("지점 이름: ");
        scanf("%s", name_input);
        for (j = 0; j < clnt_num; j++)
        {
            if (strstr(user_data_list[j].name, name_input) != NULL)
            {
                pthread_mutex_lock(&mutex);
                BD_product_list[j][i].stock -= num_input;
                strcpy(MV_Product_list[MV_cnt].name, HD_product_stock[i].name);
                MV_Product_list[MV_cnt].stock = num_input;
                cur = MV_cnt++;
                pthread_mutex_unlock(&mutex);
                show_list_visual();
                sleep(WAIT_TIME);
                write(clnt_sock_list[j], "창고반품",strlen("창고반품")); //"창고반품"메세지 보내기
                sleep(1);
                write(clnt_sock_list[j], send_buf,strlen(send_buf)); //소스이름+개수+지점이름 합친 메세지 보내기 
                HD_product_stock[i].stock += num_input;
                
                pthread_mutex_lock(&mutex);
                for(k=0; k < MV_cnt; k++){
                    if(k==cur){
                        memcpy(&MV_Product_list[k], &MV_Product_list[k + 1], sizeof(prod_data) * (MV_cnt - (k + 1))); // 소켓 리스트를 한칸씩 앞으로 당겨서 지워진 부분 매꾸기
                        break;
                    }
                }
                MV_cnt--;
                pthread_mutex_unlock(&mutex);
                
                show_list_visual();

                break;
            }
        }
        if(j>=clnt_num){
            printf("뭐라는겨?\n");
            return (void*) -1;
        }
        
    }
    return (void*) 0;
}

void recv_ctrl(int fd, char msg[])
{
    int str_len;
    char name_msg[BUF_SIZE];
    char *temp_stock;
    char *temp_num;
    char user_input[BUF_SIZE];
    int i = 0;
    void* thread_return;

    memset(&user_input, 0,BUF_SIZE);
    if (chat_mode == 1)
    {   
        //fputs(msg, stdout);
        printf("%s",msg);
        for (i = 0; i <clnt_num; i++)
        {
            if (clnt_sock_list[i] != fd){
                //printf("소켓%d\n",clnt_sock_list[i]);
                write(clnt_sock_list[i], msg,strlen(msg));  //클라이언트한테 수신한 메세지 다른 클라이언트에게 보내기 
            }
        
        }
       // write(clnt_sock_list[0], msg, BUF_SIZE);
       // write(clnt_sock_list[1], msg, BUF_SIZE);
    }
    else if (chat_mode == 2)
    {   
        printf("%s", msg);
    }
    else if (strstr(msg, "배송요청") != NULL)
    {
        //printf("fd : %d\n", fd);
        pthread_create(&t_id_list[t_id_cnt], NULL, &clnt_to_serv, (void*)&fd);
        pthread_join(t_id_list[t_id_cnt], &thread_return);
        for(i=0; i < t_id_cnt; i++){
            if(i==t_id_cnt){
                memcpy(&t_id_list[i], &t_id_list[i + 1], sizeof(pthread_t) * (t_id_cnt - (i + 1))); 
                break;
            }
        }
        t_id_cnt--;

    }
}

void *user_ctrl(void *arg)
{
    char user_input[50];
    char stock_input[50];
    char name_input[50];
    char send_buf[BUF_SIZE];
    int num_input;
    void* thread_return;
    int i = 0, j = 0;
    while (roop_ctrl)
    {
        sleep(2);
        memset(user_input, 0,BUF_SIZE);
        fgets(user_input, 50, stdin);
        //scanf("%s",user_input);
        if (chat_mode == 1)
        {
            if (strstr(user_input, "채팅종료") != NULL)
            {
                for (j = 0; j < clnt_num; j++)
                    write(clnt_sock_list[j], "채팅종료",strlen("채팅종료")); //"채팅 종료" 메세지 보냄

                printf("채팅끝낸다잉~\n");
                chat_mode = 0;
            }
            else
            {
                sprintf(send_buf, "[%s]: %s", "본점", user_input);
                for (int i = 0; i < clnt_num; i++)
                    write(clnt_sock_list[i], send_buf,strlen(send_buf)); //1:다 서버 채팅 보내기(서버에서) 
            }
        }
        else if(chat_mode==2)
        {
            if (strstr(user_input, "채팅종료") != NULL)
            {
                for (j = 0; j < clnt_num; j++)
                    write(clnt_sock_list[j], "채팅종료",strlen("채팅종료")); //"채팅 종료" 메세지 보냄 

                printf("채팅끝낸다잉~\n");
                chat_mode = 0;
            }
            else
            {
                sprintf(send_buf, "[%s]: %s", "본점", user_input);
                for (int i = 0; i < clnt_num; i++)
                    write(clnt_sock_list[i], send_buf,strlen(send_buf)); //1:1모드 서버 채팅 보내기
            }
        }
        else if (strstr(user_input, "지점배송") != NULL || strstr(user_input, "창고반품") != NULL)
        {
            pthread_create(&t_id_list[t_id_cnt], NULL, &serv_to_clnt, (void*)&user_input);
            pthread_join(t_id_list[t_id_cnt], &thread_return);
            for(i=0; i < t_id_cnt; i++){
                if(i==t_id_cnt){
                    memcpy(&t_id_list[i], &t_id_list[i + 1], sizeof(pthread_t) * (t_id_cnt - (i + 1))); 
                    break;
                }
            }
            t_id_cnt--;
        }
        /* else if (!strcmp(user_input, "q\n") || !strcmp(user_input, "Q\n"))
         {
             roop_ctrl = 0;
             break;
         }*/
        else if (strstr(user_input, "채팅시작") != NULL)
        {
            printf("채팅한다잉~\n");

            for (j = 0; j < clnt_num; j++)
            {  
                // printf("clntnum%d\n",clnt_num);
                write(clnt_sock_list[j], "채팅시작",strlen("채팅시작")); //"채팅시작"메세지 보내기
            }
           // sleep(1);
            chat_mode = 1;
            sleep(3);
     
        }    
        else if (strstr(user_input, "일대일채팅") != NULL)
        {
          //  printf("들어왔냐 일대일\n");

            printf("지점 이름을 입력해주세요\n");
            scanf("%[^\n]", name_input);
            __fpurge(stdin);
            for (j = 0; j < clnt_num; j++)
            {
                if (strstr(user_data_list[j].name, name_input) != NULL)
                {   
                    write(clnt_sock_list[j], "일대일채팅",strlen("일대일채팅")); //"일대일채팅" 메세지 보내기 
                    chat_mode=2;
                    break;
                }
            }
            if(j==clnt_num){
                printf("누구여??\n");
                continue;
            }
                
            
        }//else if
    }
}


int main(int argc, char *argv[])
{
    // 소켓통신용 변수 선언
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;

    pthread_t t_id, t_read;
    int str_len, i, j;
    // char buf[BUF_SIZE];
    char user_input[30];
    int count = 0; //임시
    char serv_input[BUF_SIZE];

    if (argc != 2) // 실행파일 경로/PORT번호 를 입력으로 받아야 함
    {
        printf("Usage : %s <port> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&mutex, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0); // TCP 소켓 생성

    /* 서버 주소정보 초기화 */
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind() error");
    }

    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen() error");
    }

    epfd = epoll_create(EPOLL_SIZE); // epoll 인스턴스 생성(관심 대상 파일 디스크립터 저장소)
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    pthread_create(&t_id, NULL, user_ctrl, (void *)&clnt_sock);
    show_list_visual();
    while (roop_ctrl)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, 300); // 이벤트가 발생할 때까지 무한대기

        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }

        for (i = 0; i < event_cnt; ++i)
        {
            if (ep_events[i].data.fd == serv_sock) // 서버에 수신된 데이터가 존재하는지 확인해 클라이언트의 연결요청이 있었는지 확인한다.
            {
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz); // 클라이언트 연결요청 수락
                if (clnt_sock == -1)
                    error_handling("accept fail");
                str_len = read(clnt_sock, user_data_list[clnt_num].name, NAME_SIZE);
                

                // 클라이언트와의 송수신을 위해 새로 생성된 소켓에 이벤트 등록
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);

                // 채팅 클라이언트 목록에 추가
                clnt_sock_list[clnt_num] = clnt_sock;
                clnt_num++;

                create_new_prod_list(clnt_num - 1);
                printf("%s지점 접속 \n", user_data_list[clnt_num - 1].name);
                printf("\n");
            }
            else // 클라이언트의 메세지를 수신하는 소켓(accept 함수로 호출된 소켓)
            {
                memset(&buf, 0, BUF_SIZE);
                str_len = read(ep_events[i].data.fd, buf, BUF_SIZE); // 클라이언트에게 데이터 수신
                buf[str_len] = 0;                                    //널문자
                if (str_len == 0)                                    // EoF 수신되었을시
                {
                    printf("%s지점 접속 종료",user_data_list[clnt_num - 1].name);
                    remove_Client(ep_events[i].data.fd);
                }
                else
                {
                    recv_ctrl(ep_events[i].data.fd, buf);
                } //작은 else
            }     //큰 else
        }
    }
    close(serv_sock); // 서버소켓 소멸
    close(epfd);      // epoll 인스턴스 소멸
}