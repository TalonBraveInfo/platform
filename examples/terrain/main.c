/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include <platform_log.h>
#include <platform_window.h>
#include <platform_graphics.h>
#include <platform_graphics_mesh.h>
#include <platform_filesystem.h>

#include <GLFW/glfw3.h>

#define TITLE "H0G Loader"
#define LOG "hog_loader"
#define PRINT(...) printf(__VA_ARGS__); plWriteLog(LOG, __VA_ARGS__);
#define WIDTH 1024
#define HEIGHT 768

typedef struct FACHeader {
    uint32_t blank[4];   // This is always blank
    uint32_t num_blocks; // Number of FACBlocks
    uint32_t unknown2;
    uint32_t unknown3;
    uint32_t unknown4;
} FACHeader;

typedef struct FACBlock {
    /* Seems to be some struct that provides an id among some other data... Probably the size of each?
     * So the id might be 65, further down the file there's a block to correspond to this.
     */
    uint32_t crap[8];
} FACBlock;

typedef struct FACTriangle {
    // ???????? ???????? ???????? ???????? I0  I1   I2  ?    ?   ?    ???????? ????????
    // 000000EB 0000023B 00090000 1B001B09 09000700 0B000D00 09000700 0B000D00 5A000000

} FACTriangle;

void load_fac_file(const char *path) {
    FACHeader header;
    memset(&header, 0, sizeof(FACHeader));

    PRINT("Opening %s\n", path);

    FILE *file = fopen(path, "r");
    if(!file) {
        PRINT("Failed to load file %s!\n", path);
        return;
    }

    if(fread(&header, sizeof(FACHeader), 1, file) != 1) {
        PRINT("Invalid file header...\n");
        goto CLEANUP;
    }

    for(int i = 0; i < plArrayElements(header.blank); i++) {
        if(header.blank[0] != 0) {
            PRINT("Invalid FAC file!\n");
            goto CLEANUP;
        }
    }

    PRINT("num_blocks: %d\n", header.num_blocks);
    PRINT("unknown2: %d\n", header.unknown2);

    if(header.num_blocks != 0) {
        FACBlock thingy0[header.num_blocks];
        if(fread(thingy0, sizeof(FACBlock), header.num_blocks, file) != header.num_blocks) {
            PRINT("Invalid thingy size!!\n");
            goto CLEANUP;
        }
        printf("Got thingies!\n");
    }

    CLEANUP:
    fclose(file);
}

typedef struct VTXCoord {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t padding;
} VTXCoord;

typedef struct PIGModel {
    VTXCoord coords[2048];

    unsigned int num_vertices;
} PIGModel;

PIGModel model;

PLMesh *load_vtx_file(const char *path) {
    FILE *file = fopen(path, "r");
    if(!file) {
        PRINT("Failed to load file %s!\n", path);
        return NULL;
    }

    model.num_vertices = (unsigned int) fread(model.coords, sizeof(VTXCoord), 2048, file);
    if(!model.num_vertices) {
        PRINT("Empty model!\n");
        fclose(file);
        return NULL;
    }

    PRINT("Vertices: %d\n", model.num_vertices);

    PLMesh *pigmesh = plCreateMesh(
            PL_PRIMITIVE_POINTS,
            PL_DRAW_IMMEDIATE,
            0,
            model.num_vertices
    );

    for(unsigned int i = 0; i < model.num_vertices; i++) {
        PRINT("%d : X(%d) Y(%d) Z(%d)\n", i,
              model.coords[i].x,
              model.coords[i].y,
              model.coords[i].z
        );
        plSetMeshVertexPosition3f(pigmesh, i, model.coords[i].x, model.coords[i].y, model.coords[i].z);
        plSetMeshVertexColour(pigmesh, i, plCreateColour4b(PL_COLOUR_RED));
    }

    plUploadMesh(pigmesh);

    return pigmesh;
}

int main(int argc, char **argv) {
    plInitialize(argc, argv, PL_SUBSYSTEM_LOG);
    plClearLog(TITLE);

    PRINT(" = = = = = = = = = = = = = = = = = = = = = = =\n");
    PRINT("   H0G Loader, created by Mark \"hogsy\" Sowden\n");
    PRINT(" = = = = = = = = = = = = = = = = = = = = = = =\n")

    if(argc < 2) {
        PRINT("Arguments:\n");
        PRINT("    -path - specifies path to model.\n");
        PRINT("    -folder - scans a directory and prints out information.\n");
    }

    memset(&model, 0, sizeof(PIGModel));

    const char *folder_arg = plGetCommandLineArgument("-folder");
    if(folder_arg && folder_arg[0] != '\0') {
        plScanDirectory(folder_arg, ".fac", load_fac_file);
    }

    const char *path_arg = plGetCommandLineArgument("-path");
    if(path_arg && path_arg[0] != '\0') {
        if(!glfwInit()) {
            plMessageBox(TITLE, "Failed to initialize GLFW!\n");
            return -1;
        }

        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, NULL, NULL);
        if(!window) {
            glfwTerminate();

            plMessageBox(TITLE, "Failed to create window!\n");
            return -1;
        }

        glfwMakeContextCurrent(window);

        plInitialize(argc, argv, PL_SUBSYSTEM_GRAPHICS);

        glfwSetWindowTitle(window, path_arg);

        char vtx_path[PL_SYSTEM_MAX_PATH] = { '\0' };
        snprintf(vtx_path, sizeof(vtx_path), "%s.vtx", path_arg);
        char fac_path[PL_SYSTEM_MAX_PATH] = { '\0' };
        snprintf(fac_path, sizeof(fac_path), "%s.fac", path_arg);
        char no2_path[PL_SYSTEM_MAX_PATH] = { '\0' };
        snprintf(no2_path, sizeof(no2_path), "%s.no2", path_arg);

        load_fac_file(fac_path);

        PLMesh *meshypiggy = load_vtx_file(vtx_path);
        if(!meshypiggy) {
            PRINT("Invalid mesh!\n");
            return -1;
        }

        plSetDefaultGraphicsState();
        plSetClearColour(plCreateColour4b(0, 0, 128, 255));

        plEnableGraphicsStates(PL_CAPABILITY_DEPTHTEST);

        PLCamera *camera = plCreateCamera();
        if(!camera) {
            PRINT("Failed to create camera!");
            return -1;
        }
        camera->mode = PL_CAMERAMODE_PERSPECTIVE;
        glfwGetFramebufferSize(window, (int *) &camera->viewport.width, (int *) &camera->viewport.height);
        camera->fov = 90.f;

        plSetCameraPosition(camera, plCreateVector3D(0, 12, -500));

        glPointSize(2.f);

        float angles = 0;
        while(!glfwWindowShouldClose(window)) {
            plClearBuffers(PL_BUFFER_COLOUR | PL_BUFFER_DEPTH | PL_BUFFER_STENCIL);

            angles += 0.5f;

            // draw stuff start
            plSetupCamera(camera);

            glLoadIdentity();
            glRotatef(angles, 0, 1, 0);

            plDrawMesh(meshypiggy);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        plDeleteCamera(camera);

        glfwTerminate();
    }

    plShutdown();

    return 0;
}