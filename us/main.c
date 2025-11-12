#include "include/genl_common.h"
#include "include/genl_debug.h"
#include "include/genl_api.h"
#include "include/genl_ops.h"
#include "include/genl_queue.h"

int family_id = 0;
uint32_t global_message_counter = 0;
int group_ids[MAX_GROUPS];
int groups_counter = 0;


int main() 
{
    int ret = 0;

    int sock_fd;
    struct sockaddr_nl sa_local;
    struct nl_req_queue *q;

    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (sock_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /* bind local pid to socket */
    memset(&sa_local, 0, sizeof(sa_local));
    sa_local.nl_family = AF_NETLINK;
    sa_local.nl_pid = getpid(); // self pid
    ret = bind(sock_fd, (struct sockaddr *)&sa_local, sizeof(sa_local));
    if (ret < 0) {
        perror("bind");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    family_id = get_family_id(sock_fd, GENLMYTEST_GENL_NAME, &sa_local);
    if (family_id < 0) {
        printf("family is not found '%s', error : %d\n", GENLMYTEST_GENL_NAME, family_id);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("family id %x for name: %s\n", family_id, GENLMYTEST_GENL_NAME);

    group_ids[groups_counter] = get_group_id(sock_fd, GENLMYTEST_GENL_NAME, GENLMYTEST_GENL_GROUP_NAME, &sa_local);
    if (group_ids[groups_counter] < 0)
    {
        printf("group is not found '%s', error : %d\n", GENLMYTEST_GENL_GROUP_NAME, family_id);
    } else  {
        groups_counter++;
    }

    printf("group id %x for name: %s\n", group_ids[groups_counter - 1], GENLMYTEST_GENL_GROUP_NAME);
    
    printf("assigning to the group %d:\n", group_ids[groups_counter - 1]);
    if (groups_counter != 0)
    {
        if (setsockopt(
            sock_fd,
            SOL_NETLINK,
            NETLINK_ADD_MEMBERSHIP,
            &group_ids[groups_counter - 1],
            sizeof(group_ids[groups_counter - 1])
        ) < 0)
    {
        perror("setsockopt NETLINK_ADD_MEMBERSHIP");
        close(sock_fd);
        return EXIT_FAILURE;
    }
    }

    q = (struct nl_req_queue *)malloc(sizeof(struct nl_req_queue));
    genl_queue_init(q);
  
    struct pollfd fds[2];
    fds[0].fd       = STDIN_FILENO;
    fds[0].events   = POLLIN;
    fds[0].revents  = 0;

    fds[1].fd       = sock_fd;
    fds[1].events   = POLLIN;
    fds[1].revents  = 0;


    while (1) {
        ret = poll(fds, 2, -1); // infinity

        if (ret < 0) {
            perror("poll error\n");
            break;
        }

        if (fds[0].revents & POLLIN) {
            char temp[MAX_LINE];
            fgets(temp, sizeof(temp), stdin);
            temp[strcspn(temp, "\n")] = '\0';

            handle_stdin_command(temp, sock_fd, sa_local, q);
        }
        
        if (fds[1].revents & POLLIN) {
            handle_socket_message(sock_fd, q);
        }
    }

    genl_queue_free(q);
    free(q);

    close(sock_fd);
    return EXIT_SUCCESS;
}