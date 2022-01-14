#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "raylib.h"
#include "raymath.h"
#include "cJSON.h"

const int screenWidth = 800;
const int screenHeight = 600;

float x = 0;
float y = 0;
float z = 0;

float rotation = 0;

float x_prev = -1;
float y_prev = -1;
float z_prev = -1;

float rotation_prev = -1;

static struct lws *web_socket = NULL;

char* msg = "UpdatePlayer | {\"pos\": {\"x\": %f, \"y\": %f, \"z\": %f}, \"rotation\": %f}";

cJSON* players;

#define RX_BUFFER_BYTES (200)

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    count += last_comma < (a_str + strlen(a_str) - 1);

    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        //assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while (*s++ = *d++);
}

static int callback_echo( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	switch( reason )
	{
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			lws_callback_on_writable( wsi );
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			char* input_str = (char*)in;

			if(strlen(input_str) > 10) {
				char** split = str_split(input_str, '|');

				if (split)
				{
					char* id = split[1];
					char* action = split[2];
					char* pos = split[3];

					if (!cJSON_IsObject(cJSON_GetObjectItemCaseSensitive(players, id))) {
						cJSON* player = cJSON_CreateObject();
						cJSON_AddNumberToObject(player, "rotation", 0);
						cJSON* player_pos = cJSON_CreateObject();
						cJSON_AddNumberToObject(player_pos, "x", 0);
						cJSON_AddNumberToObject(player_pos, "y", 0);
						cJSON_AddNumberToObject(player_pos, "z", 0);

						cJSON_AddItemToObject(player, "pos", player_pos);
						cJSON_AddItemToObject(players, id, player);
					}

					cJSON* parsed = cJSON_Parse(pos);
					cJSON* player = cJSON_GetObjectItemCaseSensitive(players, id);

					//if(action == "Move") {
						cJSON* pos_json = cJSON_GetObjectItemCaseSensitive(parsed, "pos");

						if (cJSON_IsObject(pos_json))
						{
							cJSON* x_json = cJSON_GetObjectItemCaseSensitive(pos_json, "x");
							if (cJSON_IsNumber(x_json))
							{
								double player_x = x_json->valuedouble;

								cJSON* y_json = cJSON_GetObjectItemCaseSensitive(pos_json, "y");
								if (cJSON_IsNumber(y_json))
								{
									double player_y = y_json->valuedouble;

									cJSON* z_json = cJSON_GetObjectItemCaseSensitive(pos_json, "z");
									if (cJSON_IsNumber(z_json))
									{
										double player_z = z_json->valuedouble;

										
										cJSON* player_pos = cJSON_CreateObject();
										cJSON_AddNumberToObject(player_pos, "x", player_x);
										cJSON_AddNumberToObject(player_pos, "y", player_y);
										cJSON_AddNumberToObject(player_pos, "z", player_z);

										cJSON_ReplaceItemInObject(player, "pos", player_pos);
										cJSON_ReplaceItemInObject(players, id, player);
									}
								}
							}
						}

						cJSON* rotation_json = cJSON_GetObjectItemCaseSensitive(parsed, "rotation");
						if (cJSON_IsNumber(rotation_json))
						{
							cJSON_ReplaceItemInObject(player, "rotation", rotation_json);
						}
					//}

					free(split);
				}
			}

			lws_callback_on_writable( wsi );
			break;

		case LWS_CALLBACK_CLIENT_WRITEABLE:
		{
            unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
            unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
            size_t n = sprintf( (char *)p, msg, x, y, z, rotation );

			if(x == x_prev && y == y_prev && z == z_prev && rotation == rotation_prev) {
				n = 0;
			} else {
				x_prev = x;
				y_prev = y;
				z_prev = z;
				rotation_prev = rotation;
			}

            lws_write( wsi, p, n, LWS_WRITE_TEXT );

			break;
		}

		case LWS_CALLBACK_CLOSED:
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			web_socket = NULL;
			break;

		default:
			break;
	}

	return 0;
}

enum protocols
{
	PROTOCOL_ECHO = 0,
	PROTOCOL_COUNT
};

static struct lws_protocols protocols[] =
{
	{
		"echo-protocol",
		callback_echo,
		0,
		RX_BUFFER_BYTES,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

int main( int argc, char *argv[] )
{
	players = cJSON_CreateObject();

	struct lws_context_creation_info info;
	memset( &info, 0, sizeof(info) );

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	struct lws_context *context = lws_create_context( &info );

	time_t old = 0;

    InitWindow(screenWidth, screenHeight, "game");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 1.75f, 0.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;                   // Camera mode type

    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    SetCameraMode(camera, CAMERA_FIRST_PERSON);

	Model steve = LoadModel("assets/Steve.gltf");
	Model grass = LoadModel("assets/Grass.gltf");

	steve.transform = MatrixRotateXYZ((Vector3) { 90 * DEG2RAD, 0, 0 });
	grass.transform = MatrixRotateXYZ((Vector3) { 90 * DEG2RAD, 0, 0 });

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
		UpdateCamera(&camera);

		x = camera.position.x;
		y = camera.position.y;
		z = camera.position.z;

		Matrix cam_mat = GetCameraMatrix(camera);

		rotation = cam_mat.m0;

        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

				DrawModel(grass, (Vector3) { 3, 0, 3 }, 0.5, WHITE);
				DrawModel(grass, (Vector3) { 3, 1, 3 }, 0.5, WHITE);
                
				cJSON *player = NULL;

				cJSON_ArrayForEach(player, players)
				{
					double player_x;
					double player_y;
					double player_z;
					double rotation;

					cJSON* pos_json = cJSON_GetObjectItemCaseSensitive(player, "pos");

					if (cJSON_IsObject(pos_json))
					{
						cJSON* x_json = cJSON_GetObjectItemCaseSensitive(pos_json, "x");
						if (cJSON_IsNumber(x_json))
						{
							cJSON* y_json = cJSON_GetObjectItemCaseSensitive(pos_json, "y");
							if (cJSON_IsNumber(y_json))
							{
								cJSON* z_json = cJSON_GetObjectItemCaseSensitive(pos_json, "z");
								if (cJSON_IsNumber(z_json))
								{
									player_x = x_json->valuedouble;
									player_y = y_json->valuedouble - 1; 
									player_z = z_json->valuedouble;
								}
							}
						}
					}

					cJSON* rotation_json = cJSON_GetObjectItemCaseSensitive(player, "rotation");
					if (cJSON_IsNumber(rotation_json))
					{
						steve.transform = MatrixRotateXYZ((Vector3) { 90 * DEG2RAD, (-((rotation_json->valuedouble + 1) / 2)) * 180 * DEG2RAD, 0 });

						DrawModel(steve, (Vector3) { player_x, player_y, player_z }, 0.065, WHITE);
						//DrawModelEx(steve, (Vector3) { player_x, player_y, player_z }, (Vector3) {0, 1, 0}, rotation_json->valuedouble, (Vector3) {0.065, 0.065, 0.065}, WHITE);
					}
				}

				DrawGrid(10, 1);

            EndMode3D();

			DrawText(cJSON_Print(players), 10, 10, 20, BLACK);
        EndDrawing();
        
		struct timeval tv;
		gettimeofday( &tv, NULL );

		/* Connect if we are not connected to the server. */
		if( !web_socket && tv.tv_sec != old )
		{
			struct lws_client_connect_info ccinfo = {0};
			ccinfo.context = context;
			ccinfo.address = "localhost";
			ccinfo.port = 8080;
			ccinfo.path = "/";
			ccinfo.host = lws_canonical_hostname( context );
			ccinfo.origin = "origin";
			ccinfo.protocol = protocols[PROTOCOL_ECHO].name;
			web_socket = lws_client_connect_via_info(&ccinfo);
		}

		lws_service( context, 250 );
	}

	lws_context_destroy( context );

    CloseWindow();

	return 0;
}