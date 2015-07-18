// Copyright 2015 Ian Kronquist.
#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <dirent.h>

// Define spec'd constants.
#define MIN_CONN 3
#define MAX_CONN 6
#define NUM_ROOMS 7
#define NUM_ROOM_NAMES 10
#define NAME_BUFFER_LEN 12

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

struct room rooms_list[NUM_ROOMS];

void repl();

struct room *generate_rooms();
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

void repl() {
        struct room *current_room = &rooms_list[0];
        while (true) {
                printf("a");
                print_all_connections(current_room);
        }
}

struct room *generate_rooms() {
        bool taken_names[NUM_ROOMS];
        memset(&taken_names, 0, NUM_ROOMS * sizeof(bool));
        for (int i = 0; i < NUM_ROOMS; i++) {
                while (true) {
                        unsigned int room_index = rand() % NUM_ROOMS;
                        if (!taken_names[room_index]) {
                                taken_names[room_index] = true;
                                break;
                        }
                        rooms_list[i].name = room_names[room_index];
                }

                rooms_list[i].type = MID_ROOM;

                unsigned int num_conns = rand() % (MAX_CONN - MIN_CONN);
                num_conns += MIN_CONN;
                for (int j = 0; j < num_conns; j++) {
                        unsigned int random_room = rand() % NUM_ROOMS;
                        while (!connect(i, random_room, rooms_list)) { }
                }
        }
        rooms_list[0].type = START_ROOM;
        rooms_list[NUM_ROOMS - 1].type = END_ROOM;
        return rooms_list;
}

bool connect(unsigned int room1, unsigned int room2,
                struct room rooms_list[NUM_ROOMS]) {
        if (rooms_list[room1].num_conns >= MAX_CONN ||
                        rooms_list[room2].num_conns >= MAX_CONN) {
                puts("1");
                exit(1);
                STUCK HERE
                look at cap conns
                return false;
        }
        for (int i = 0; i < rooms_list[room1].num_conns; i++) {
                if (rooms_list[room1].connections[i] == &rooms_list[room2]) {
                        puts("2");
                        exit(2);
                        return false;
                }
        }
        int r1cons = rooms_list[room1].num_conns;
        int r2cons = rooms_list[room2].num_conns;
        rooms_list[room1].connections[r1cons-1] = &rooms_list[room2];
        rooms_list[room2].connections[r2cons-1] = &rooms_list[room1];
        return true;
}

char **get_dir_name() {
        pid_t pid = getpid();

        uid_t uid = getuid();
        struct passwd *user_info = getpwuid(uid);
        char **dir_name;
        asprintf(dir_name, "%s.rooms.%d", user_info->pw_name, pid);
        free(user_info);
        return dir_name;

}

void serialize_rooms(struct room rooms[NUM_ROOMS]) {
        char **dir_name = get_dir_name();
        mkdir(*dir_name, 0777);
        chdir(*dir_name);
        for (int i = 0; i < NUM_ROOMS; i++) {
                FILE *fp = fopen(rooms[i].name, "w");
                fprintf(fp, "ROOM NAME: %s\n", rooms[i].name);

                char **connection_name;
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
        free(*dir_name);
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

void print_all_connections_helper(struct room *r) {
        printf("%s\n", r->name);
}

void print_all_connections(struct room *r) {
        iter_rooms(r, &print_all_connections_helper);
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
        char **dir_name = get_dir_name();
        // FIXME: Make sure that directory exists with stat
        chdir(*dir_name);
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

        free(*dir_name);
        chdir("..");
        return rooms;
}

void destroy_rooms(struct room *rooms) {
        // Take advantage of the fact that the rooms are allocated as a
        // contiguous array.
        free(rooms);
}

