import os

def create_assets_h():
    assets = []
    path = "./src/assets"
    path_parts = path.split('/')
    path_parts.pop(0)
    for x in os.listdir(path):
        if (not x.endswith(".h")) and (not x.endswith(".spv")):
            asset_things = x.split('.')
            if(asset_things[1] == 'glsl'):
                asset_things[1] = 'spv'
            assets.append(asset_things)
    out = """#ifndef ASSETS_GEN
#define ASSETS_GEN

#ifdef DEBUG"""
    for asset in assets:
        iter_path = path_parts.copy()
        iter_path.append(asset[0])
        out += f"\n    #define "+ "_".join(map(lambda x : x.upper(), iter_path))+" \"./"+"/".join(iter_path)+"."+asset[1]+"\""
    out +="""
#else"""
    for asset in assets:
        iter_path = path_parts.copy()
        iter_path.append(asset[0])
        out += f"\n\n    #include \"../"+"/".join(iter_path)+".h\""
        out += f"\n    #define " + "_".join(map(lambda x : x.upper(), iter_path))+ " __" + "_".join(iter_path)+"_"+asset[1]
        out += f"\n    #define " + "_".join(map(lambda x : x.upper(), iter_path))+"_LEN" + " __" + "_".join(iter_path)+"_"+asset[1]+"_len"
    out += """
#endif

#endif
"""
    with open('./src/assets.h', 'w') as file:
        file.write(out)

def create_build_release_assets_sh():
    assets = []
    path = "./src/assets"
    path_parts = path.split('/')
    path_parts.pop(0)
    for x in os.listdir(path):
        if (not x.endswith(".h")) and (not x.endswith(".glsl")):
            asset_things = x.split('.')
            assets.append(asset_things)
    out = """#!/bin/sh
set -xe
"""
    for asset in assets:
        iter_path = path_parts.copy()
        iter_path.append(asset[0])
        out += "\nxxd -i \"./"+"/".join(iter_path)+"."+asset[1]+"\" > \"./"+"/".join(iter_path)+".h\""

    with open('./build-release-assets.sh', 'w') as file:
        file.write(out)

if __name__ == "__main__":
    create_assets_h()
    create_build_release_assets_sh()