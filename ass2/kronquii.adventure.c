// Copyright 2015 Ian Kronquist.
#define _GNU_SOURCE
#include <dirent.h>
#include <math.h>
#include <pwd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

// Define spec'd constants.
#define MIN_CONN 3
#define MAX_CONN 6
#define NUM_ROOMS 7
#define NUM_ROOM_NAMES 10
#define NAME_BUFFER_LEN 12

// The list of room names.
const char *room_names[NUM_ROOM_NAMES] = {
        "arrakis", // Herbet's Dune
        "tatooine", // Lucas' Star Wars
        "winter", // LeGuin's The Left Hand of Darkness
        "trantor", // Asimov's Foundation
        "lusitania", // Card's Xenophobia
        "nasqueron", // Banks' The Algebraist
        "caprica", // Larson's Battlestart Galactica
        "camazotz", // L'Engle's A Wrinkle In Time
        "disk world", // Pratchet's Disk World
        "magrathea" // Adams' Hitchhiker's Guide to The Galaxy
};

// The three types of rooms.
enum room_type {
        START_ROOM,
        END_ROOM,
        MID_ROOM
};

// The room struct. A single node in the graph.
struct room {
        enum room_type type;
        const char *name;
        unsigned int cap_conns;
        unsigned int num_conns;
        struct room *connections[NUM_ROOMS];
};
struct room rooms_list[NUM_ROOMS];

void repl();

struct room *generate_rooms();
void print_room(unsigned int room);
int create_rooms_dir(char *dir_name);
void serialize_rooms(struct room rooms[NUM_ROOMS]);
bool connect(unsigned int room1, unsigned int room2,
                struct room rooms_list[NUM_ROOMS]);

void print_all_connections(struct room *r);
// Returns a pointer to the starting room which forms a graph.
struct room* deserialize_rooms();

int main() {
        // Seed the random number generator with the current time.
        // Don't do this for anything serious like crypto.
        srand(time(0));
        generate_rooms();
        serialize_rooms(rooms_list);
        deserialize_rooms();
        repl();
}

void ending_message(unsigned int num_steps, struct room **visited,
                    unsigned int visited_index) {

        printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
        printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", num_steps);
        for (int i = 0; i < visited_index; i++) {
                printf("%s\n", visited[i]->name);
        }
}

void repl() {
        struct room *current_room = &rooms_list[0];
        struct room **visited = malloc(sizeof(struct room*) * NUM_ROOMS);
        unsigned int visited_index = 0;
        unsigned int visited_cap = NUM_ROOMS;
        char buffer[NAME_BUFFER_LEN];
        unsigned int num_steps = 0;
        while (true) {
                Top:
                if (current_room->type == END_ROOM) {
                        ending_message(num_steps, visited, visited_index);
                        return;
                }

                printf("\nCURRENT LOCATION: %s\n", current_room->name);
                print_all_connections(current_room);
                printf("WHERE TO? >");
                char *ret = fgets(buffer, NAME_BUFFER_LEN, stdin);
                assert(ret != NULL);
                buffer[strlen(buffer)-1] = '\0';
                for (int i = 0; i < current_room->num_conns; i++) {
                        if (strncmp(buffer, current_room->connections[i]->name,
                                    NAME_BUFFER_LEN) == 0) {
                                current_room = current_room->connections[i];

                                if (visited_index >= visited_cap) {
                                        visited_cap += NUM_ROOMS *
                                                       sizeof(struct room*);
                                        visited = realloc(visited,
                                                          visited_cap);
                                        assert(visited != NULL);
                                }
                                visited[visited_index] = current_room;
                                visited_index++;
                                num_steps++;

                                goto Top;
                        }
                }
                printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
        }
}

struct room *generate_rooms() {
        bool taken_names[NUM_ROOMS];
        memset(&taken_names, 0, NUM_ROOMS * sizeof(bool));
        for (int i = 0; i < NUM_ROOMS; i++) {
                rooms_list[i].num_conns = 0;
                unsigned int cap_conns = rand() % (MAX_CONN - MIN_CONN);
                cap_conns += MIN_CONN;
                rooms_list[i].cap_conns = cap_conns;
                while (true) {
                        unsigned int room_index = rand() % NUM_ROOMS;
                        if (!taken_names[room_index]) {
                                taken_names[room_index] = true;
                                rooms_list[i].name = room_names[room_index];
                                break;
                        }

                }
                rooms_list[i].type = MID_ROOM;
        }

        for (int i = 0; i < NUM_ROOMS; i++) {
                for (int j = 0; j < rooms_list[i].cap_conns; j++) {
                        unsigned int random_room = rand() % NUM_ROOMS;
                        while (!connect(i, random_room, rooms_list)) {
                                random_room = rand() % NUM_ROOMS;
                        }
                }
        }
        rooms_list[0].type = START_ROOM;
        rooms_list[NUM_ROOMS - 1].type = END_ROOM;
        return rooms_list;
}

bool already_connected(unsigned int room1, unsigned int room2) {
        if (room1 == room2) {
                return true;
        }
        for (int i = 0; i < rooms_list[room1].num_conns; i++) {
                if (rooms_list[room1].connections[i] == &rooms_list[room2] &&
                    rooms_list[room1].connections[i] != NULL) {
                        return true;
                }
        }
        return false;
}

bool connect(unsigned int room1, unsigned int room2,
                struct room rooms_list[NUM_ROOMS]) {
        struct room *r1 = &rooms_list[room1];
        struct room *r2 = &rooms_list[room2];
        if (r1->num_conns == MAX_CONN) {
                return true;
        }
        if (already_connected(room1, room2)) {
                return false;
        }
        if (r1->num_conns >= MAX_CONN || r2->num_conns >= MAX_CONN) {
                return false;
        }
        assert(r1 != NULL);
        assert(r2 != NULL);
        r1->connections[r1->num_conns] = r2;
        r2->connections[r2->num_conns] = r1;
        r1->num_conns++;
        r2->num_conns++;
        assert(r1->connections[r1->num_conns-1] != NULL);
        assert(r2->connections[r2->num_conns-1] != NULL);
        return true;
}

void print_room(unsigned int room) {
        printf("name: %s,\nnum_conns %d,\ncap_conns %d,\n",
                rooms_list[room].name,
                rooms_list[room].num_conns,
                rooms_list[room].cap_conns
        );
        for (int i = 0; i < rooms_list[room].num_conns; i++) {
                printf("connection: %s\n", rooms_list[room].connections[i]->name);
        }
}

char *get_dir_name() {
        pid_t pid = getpid();

        uid_t uid = getuid();
        struct passwd *user_info = getpwuid(uid);
        unsigned int buffer_max_len = strlen(".rooms.") +
                                      strlen(user_info->pw_name) + log10(pid);
        char *dir_name = malloc(buffer_max_len * sizeof(char));
        assert(dir_name != NULL);
        sprintf(dir_name, "%s.rooms.%d", user_info->pw_name, pid);
        //free(user_info);
        return dir_name;
}

void serialize_rooms(struct room rooms[NUM_ROOMS]) {
        char *dir_name = get_dir_name();
        mkdir(dir_name, 0777);
        chdir(dir_name);
        for (int i = 0; i < NUM_ROOMS; i++) {
                FILE *fp = fopen(rooms[i].name, "w");
                fprintf(fp, "ROOM NAME: %s\n", rooms[i].name);

                for (int j = 0; j < rooms[i].num_conns; j++) {
                        fprintf(fp, "CONNECTION %d: %s\n", j + 1, rooms[j].name);
                }

                switch (rooms[i].type) {
                        case END_ROOM:
                                fprintf(fp, "ROOM TYPE: END_ROOM");
                                break;
                        case MID_ROOM:
                                fprintf(fp, "ROOM TYPE: MID_ROOM");
                                break;
                        case START_ROOM:
                                fprintf(fp, "ROOM TYPE: START_ROOM");
                                break;
                }
                fclose(fp);
        }
        chdir("..");
        free(dir_name);
}

const char *pick_right_name(char *in) {
        for (int i = 0; i < MAX_CONN; i++) {
                if (strcmp(in, room_names[i]) == 0) {
                        return room_names[i];
                }
        }
        return NULL;
}

struct room *pick_right_room(char *in) {
        for (int i = 0; i < NUM_ROOMS; i++) {
                if (strcmp(in, rooms_list[i].name) == 0) {
                        return &rooms_list[i];
                }
        }
        return NULL;
}

void iter_rooms(struct room *r, void (*func)(struct room *)) {
        for (int i = 0; i < r->num_conns; i++) {
                func((r->connections[i]));
        }
}

void print_all_connections(struct room *r) {
        printf("POSSIBLE CONNECTIONS: ");
        for (int i = 0; i < r->num_conns-1; i++) {
                printf("%s, ", r->connections[i]->name);
        }
        if (r->num_conns > 0) {
                printf("%s.\n", r->connections[r->num_conns-1]->name);
        }
}

struct room deserialize_single_room(char *name) {
        struct room r;
        FILE *file = fopen(name, "r");

        // I control the names so I know how long to make this buffer
        // should be as long as the longest name or START_ROOM + 1 more
        // character for \0.
        char received_name[NAME_BUFFER_LEN];
        fscanf(file, "ROOM NAME: %s\n", name);
        r.name = pick_right_name(name);

        int read;
        int conn_number;
        while ((read =
                fscanf(file, "CONNECTION %d: %s\b", &conn_number, received_name)) != 0
                && read != EOF) {
                r.connections[conn_number-1] = pick_right_room(received_name);
        }
        r.num_conns = conn_number - 1;
        fscanf(file, "ROOM TYPE: %s\n", received_name);
        if (strcmp(name, "START_ROOM") == 0) {
                r.type = START_ROOM;
        } else if (strcmp(name, "END_ROOM") == 0) {
                r.type = END_ROOM;
        } else if (strcmp(name, "MID_ROOM") == 0) {
                r.type = MID_ROOM;
        } else {
                // What the hell are you doing?
        }
        return r;
}

struct room* deserialize_rooms() {
        struct room *rooms = malloc(NUM_ROOMS * sizeof(struct room));
        unsigned int room_count = 0;
        char *dir_name = get_dir_name();
        // FIXME: Make sure that directory exists with stat
        chdir(dir_name);
        DIR *dp;
        struct dirent *dir;
        dp = opendir (".");

        if (dp != NULL) {
                while ((dir = readdir (dp))) {
                        rooms[room_count] = deserialize_single_room(dir->d_name);
                }

                closedir (dp);
        }
        else {
                //perror ("Couldn't open the directory");
                puts("test");
        }

        free(dir_name);
        chdir("..");
        return rooms;
}

void destroy_rooms(struct room *rooms) {
        // Take advantage of the fact that the rooms are allocated as a
        // contiguous array.
        free(rooms);
}

