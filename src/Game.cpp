#include "Game.h"
#include "assets.h"
#include <vector>
#include <random>
#include <time.h>

#ifdef DEBUG
#include <stdio.h>
#endif

const float mouse_sensitivity = 0.002;

EXPORT_FN void init_game(Game* gameIN){
    if(game != gameIN) game = gameIN;
    srand(time(nullptr));
}

MeshView* plane;
void draw_plane_tiled(vec3 origin, vec2 size, float tilingSize, Texture* texture, quat rotation = {0,0,1,0}) {
    mat4 translation = {
        1, 0, 0, origin.x,
        0, 1, 0, origin.y,
        0, 0, 1, origin.z,
        0, 0, 0, 1
    };

    mat4 scaling = {
        size.x, 0,      0, 0,
        0,      1,      0, 0,
        0,      0, size.y, 0,
        0,      0,      0, 1
    };

    mat4 rotationMatrix = rotation.toMatrix();
    mat4 modelMatrix = mat4::mul(translation, mat4::mul(rotationMatrix, scaling));
    game->callbacks.drawMesh(plane, texture, {modelMatrix, {size.x / tilingSize, size.y / tilingSize}});
}

MeshView* cube;
Texture* testTexture;
void drawCalcCell(int number, vec3 origin, float size = 0.5f){
    game->callbacks.drawMesh(cube,testTexture,{
        {
            size,0,0,origin.x,
            0,size,0,origin.y,
            0,0,size,origin.z,
            0,0,0,1,
        },
        {1.0f/4.0f,1.0f/4.0f}, {(float)(number % 4) / 4.0f + 0.25f,(float)(number / 4)  / 4.0f + 0.25f}
    });
}

bool anyCellClicked = true;

struct Cell;

bool second = false;

std::vector<Cell> writtenNumber1;
std::vector<Cell> writtenNumber2;


size_t operatorr = 0;

bool divided_by_zero = false;

vec3 cells_vel[] = {
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
};

struct Cell{
    int number;
    vec3 origin;
    bool clicked;
    bool appeared = false;

    float target_pos;
    float pos;
    float original_pos;

    float target_size = 0.5f;
    float size = 0.0f;

    int getNumber(std::vector<Cell>& arr) {
        if (arr.size() == 1) return arr[0].number;

        int outNumber = 0;
        int pos = 1;

        bool isNegative = (arr[0].number == 11);
        
        for (int i = arr.size() - 1; i >= (isNegative ? 1 : 0); --i) {
            if (arr[i].number < 0 || arr[i].number > 9) {
                return 0;
            }
            outNumber += arr[i].number * pos;
            pos *= 10;
        }
        if (isNegative) {
            outNumber *= -1;
        }

        return outNumber;
    }

    std::vector<Cell> genNumber(int num) {
        std::vector<Cell> out;
        bool isNegative = num < 0;
        if (isNegative) num = -num;

        std::vector<int> digits;

        if (num == 0) {
            digits.push_back(0);
        } else {
            while (num > 0) {
                digits.push_back(num % 10);
                num /= 10;
            }
        }

        if (isNegative) {
            out.push_back(Cell(11, origin));
        }

        for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
            out.push_back(Cell(*it, origin));
        }

        return out;
    }

    Cell(int number, vec3 origin): number(number), origin(origin) {
        target_pos = origin.z;
        pos = origin.z;
        original_pos = origin.z;
    }

    float getRandVal(){
        return (float)rand()/RAND_MAX - 0.5f;
    }

    void draw(float deltaTime, float speed = 4.0f){
        if(appeared == false){
            size = size + (target_size - size) * speed * deltaTime;
            drawCalcCell(number, {origin.x, origin.y, pos}, size);

            if(target_size - size < 0.01f) {
                appeared = true;
                anyCellClicked = false;
            }
            return; 
        }


        if(clicked) {
            anyCellClicked = true;
            target_pos = original_pos + 2;
        }

        if(clicked && target_pos - pos < 0.1f){
            clicked = false;
            anyCellClicked = false;
            target_pos = original_pos;
            if(number < 10){
                if(!second){
                    writtenNumber1.push_back(Cell(number, origin));
                }else{
                    writtenNumber2.push_back(Cell(number, origin));
                }
            }
            if(number == 14){
                writtenNumber1.clear();
                writtenNumber2.clear();
                second = false;
            }

            if(number >= 10 && number < 14){
                if(writtenNumber1.size() > 0){
                    operatorr = number;
                    if(second == false){
                        second = true;
                    }
                }
            }

            if(number == 15 && (writtenNumber1.size() != 0 && writtenNumber2.size() != 0)){
                int a = getNumber(writtenNumber1);
                int b = getNumber(writtenNumber2);
                DEBUG_MSG("a = %d b = %d\n", a,b);

                if(b == 0 && operatorr == 13){
                    writtenNumber2.clear();
                    writtenNumber1.clear();
                    second = false;
                    
                    divided_by_zero = true;
                    for(size_t i = 0; i < 16; i++){
                        cells_vel[i] = {getRandVal() * getRandVal() * 5,getRandVal() * getRandVal() * 5 - 4.0f,getRandVal() * getRandVal() * 5};
                    }
                    return;
                }

                writtenNumber2.clear();
                writtenNumber1.clear();
                second = false;

                int c = 0;
                if(operatorr == 10){
                    c = a+b;
                }
                if(operatorr == 11){
                    c = a-b;
                }
                if(operatorr == 12){
                    c = a*b;
                }
                if(operatorr == 13){
                    c = a/b;
                }

                DEBUG_MSG("c = %d\n", c);

                writtenNumber1 = genNumber(c);
            }


        }

        pos = pos + (target_pos - pos) * 10 * deltaTime;

        drawCalcCell(number, {origin.x, origin.y, divided_by_zero ? origin.z : pos});
    }
};


int mod(int a, int b)
{
   if(b < 0)
     return -mod(-a, -b);   
   int ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}

vec3 offset = {-0.5f,.5, -4};

vec3 cells_pos[] = {
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
    {0,0,0},
};

Cell cells[] = {
    Cell(1,{-1 + offset.x,-1 + offset.y,offset.z}),
    Cell(2,{0 + offset.x,-1 + offset.y,offset.z}),
    Cell(3,{1 + offset.x,-1 + offset.y,offset.z}),
    Cell(13,{2 + offset.x,-1 + offset.y,offset.z}),

    Cell(4,{-1 + offset.x,0 + offset.y,offset.z}),
    Cell(5,{0 + offset.x,0 + offset.y,offset.z}),
    Cell(6,{1 + offset.x,0 + offset.y,offset.z}),
    Cell(12,{2 + offset.x,0 + offset.y,offset.z}),

    Cell(7,{-1 + offset.x,1 + offset.y,offset.z}),
    Cell(8,{0 + offset.x,1 + offset.y,offset.z}),
    Cell(9,{1 + offset.x,1 + offset.y,offset.z}),
    Cell(11,{2 + offset.x,1 + offset.y,offset.z}),

    Cell(14,{-1 + offset.x,2 + offset.y,offset.z}),
    Cell(0,{0 + offset.x,2 + offset.y,offset.z}),
    Cell(15,{1 + offset.x,2 + offset.y,offset.z}),
    Cell(10,{2 + offset.x,2 + offset.y,offset.z})
};

int cursor = 0;
float coolPos = -2.0f;
EXPORT_FN void update_game(double deltaTime, Game* gameIN){
    if(game != gameIN) game = gameIN;
    Window* window = game->window;
    plane = game->callbacks.getModel(SRC_ASSETS_PLANE);
    cube = game->callbacks.getModel(SRC_ASSETS_UNTITLED);

    game->ubo->view = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    mat4 rot = {
        cosf(game->yaw),0,sinf(game->yaw),0,
        0,1,0,0,
        -sinf(game->yaw),0,cosf(game->yaw),0,
        0,0,0,1
    };

    mat4 rot2 = {
        1,0,0,0,
        0,cosf(game->pitch),-sinf(game->pitch),0,
        0,sinf(game->pitch),cosf(game->pitch),0,
        0,0,0,1
    };

    game->ubo->view = mat4::mul(mat4::mul(rot2,rot), game->ubo->view);

    game->Time += deltaTime;

    testTexture = game->callbacks.getTexture(SRC_ASSETS_NUMBERS);
    Texture* selectTexture = game->callbacks.getTexture(SRC_ASSETS_SELECT);
    Texture* wall = game->callbacks.getTexture(SRC_ASSETS_WALL2);
    Texture* author = game->callbacks.getTexture(SRC_ASSETS_AUTHOR);

    float texture_scale = 10.0f;

    draw_plane_tiled({0.0f,0.0f,-50.0f},{300.0f,300.0f},texture_scale,wall, quat::fromEuler(PI + PI/2,0,PI + PI/2));

    if(divided_by_zero == false){
        if(game->window->just_pressed_keys[VK_LEFT] || game->window->just_pressed_keys['A']) if(cursor % 4 > 0) cursor -= 1;
        if(game->window->just_pressed_keys[VK_RIGHT] || game->window->just_pressed_keys['D']) if(cursor % 4 < 3) cursor += 1;
        if(game->window->just_pressed_keys[VK_UP] || game->window->just_pressed_keys['W']) cursor = mod(cursor - 4, 16);
        if(game->window->just_pressed_keys[VK_DOWN] || game->window->just_pressed_keys['S']) cursor = mod(cursor + 4, 16);
        if(game->window->just_pressed_keys[VK_SPACE] || game->window->just_pressed_keys[VK_RETURN]) if(anyCellClicked == false) cells[cursor].clicked = true;
    }
    if(game->window->just_pressed_keys[VK_F11]) game->fullscreen = !game->fullscreen;

    for(size_t i = 0; i< 16; i++){
        if(divided_by_zero){
            cells_vel[i] += vec3(0,9.8,0) * deltaTime;
            cells[i].origin += cells_vel[i] * deltaTime;
        }
        cells[i].draw(deltaTime, 3.0f);
    }

    float original_pos = -2.0f;

    for(size_t index = 0; index < writtenNumber1.size(); index++){
        auto cell = writtenNumber1[index];

        float targetPos = second ? original_pos - 1 : original_pos;
        coolPos = coolPos + (targetPos - coolPos) * 2.0f * deltaTime;

        drawCalcCell(cell.number,{(float)writtenNumber1.size() / -2.0f + (float)index + 0.5f,coolPos,-4.0});
    }

    for(size_t index = 0; index < writtenNumber2.size(); index++){
        auto cell = writtenNumber2[index];
        drawCalcCell(cell.number,{(float)writtenNumber2.size() / -2.0f + (float)index + 0.5f,original_pos,-4.0});
    }

    if(divided_by_zero == false){
        game->callbacks.drawMesh(cube, selectTexture,{
            {
                .6,0,0,offset.x - 1 + cursor  % 4,
                0,.6,0,offset.y - 1 + cursor / 4,
                0,0,.6,offset.z,
                0,0,0,1,
            }
        });
    }

    if(divided_by_zero == true){
        game->callbacks.drawMesh(cube, author,{
                {
                    .5*4,0,0,0,
                    0,.5,0, 0,
                    0,0,.5,-5,
                    0,0,0,1,
                }
        });
    }
    

}