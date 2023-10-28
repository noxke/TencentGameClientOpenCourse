import frida

# 开启无敌
invincible_js = """
Java.perform(() => {
    var libil2cpp = Process.findModuleByName("libil2cpp.so");
    // OnTriggerEnter2D
    var offset1 = 0x5E2BCC;
    var addr1 = libil2cpp.base.add(offset1);
    var arr1 = [0xE5, 0x00, 0x00, 0xEA]; // B 0x5E2F68
    // 修改内存保护
    Memory.protect(addr1, 0x1000, 'rwx');
    Memory.writeByteArray(addr1, arr1);
    // OnCollisionEnter2D
    var offset2 = 0x5E30CC;
    var addr2 = libil2cpp.base.add(offset2);
    var arr2 = [0x40, 0x00, 0x00, 0xEA]; // B 0x5E31D4
    // 修改内存保护
    Memory.protect(addr2, 0x1000, 'rwx');
    Memory.writeByteArray(addr2, arr2);
});
"""

# 关闭无敌
off_invincible_js = """
Java.perform(() => {
    var libil2cpp = Process.findModuleByName("libil2cpp.so");
    // OnTriggerEnter2D
    var offset1 = 0x5E2BCC;
    var addr1 = libil2cpp.base.add(offset1);
    var arr1 = [0xE5, 0x00, 0x00, 0x1A]; // BNE 0x5E2F68
    // 修改内存保护
    Memory.protect(addr1, 0x1000, 'rwx');
    Memory.writeByteArray(addr1, arr1);
    // OnCollisionEnter2D
    var offset2 = 0x5E30CC;
    var addr2 = libil2cpp.base.add(offset2);
    var arr2 = [0x18, 0xB0, 0x8D, 0xE2]; // ADD R11, SP, #0x18
    // 修改内存保护
    Memory.protect(addr2, 0x1000, 'rwx');
    Memory.writeByteArray(addr2, arr2);
});
"""


# 开启飞行
fly_js = """
Java.perform(() => {
    var libil2cpp = Process.findModuleByName("libil2cpp.so");
    // 在LateUpdate函数中有设置重力的部分, 将数值修改为0.125减轻重力的影响
    var offset1 = 0x5E289C;
    var addr1 = libil2cpp.base.add(offset1);
    var arr1 = [0x3D, 0x14, 0xA0, 0xE3]; // MOV R1. #0x3D000000 0.03125
    // 修改内存保护
    Memory.protect(addr1, 0x1000, 'rwx');
    Memory.writeByteArray(addr1, arr1);
    var offset2 = 0x5E2A38; 
    var addr2 = libil2cpp.base.add(offset2);
    var arr2 = [0x3D, 0x14, 0xA0, 0xE3]; // MOV R1. #0x3D000000 0.03125
    // 修改内存保护
    Memory.protect(addr1, 0x1000, 'rwx');
    Memory.writeByteArray(addr1, arr1);
    // 将tiltSmooth修改为maxTiltSmooth的分支patch掉
    var offset3 = 0x5E2A14; 
    var addr3 = libil2cpp.base.add(offset3);
    var arr3 = [0x0A, 0x00, 0x00, 0xEA]; // B 0x5E2A44
    // 修改内存保护
    Memory.protect(addr3, 0x1000, 'rwx');
    Memory.writeByteArray(addr3, arr3);
    // 还需要将thrust推力减小
    // 由于不能直接拿到PlayerController的实例,需要先hook Update, 获得实例之后才能继续修改
    var pc_instance = NULL;
    var func_offset = 0x5E23E4; // PlayerController__Update
    var func_addr = libil2cpp.base.add(func_offset);
    // 函数对象
    var targetFunc = new NativeFunction(func_addr, 'void', ['pointer']);
    // hook
    Interceptor.attach(targetFunc, {
        onEnter: function (args) {
            // send(args[0]);
            pc_instance = args[0];
            Interceptor.detachAll();
            send(`PlayerController Instance: ${pc_instance}`);
            var thrust_ptr = pc_instance.add(0xC); // 偏移0xC
            Memory.writeFloat(thrust_ptr, 50);
            var thrust = Memory.readFloat(thrust_ptr);
            send(thrust);
        }
    });
});
"""

# 关闭飞行
off_fly_js = """
Java.perform(() => {
    var libil2cpp = Process.findModuleByName("libil2cpp.so");
    // 在LateUpdate函数中有设置重力的部分, 恢复其数值
    var offset1 = 0x5E289C;
    var addr1 = libil2cpp.base.add(offset1);
    var arr1 = [0xFE, 0x15, 0xA0, 0xE3]; // MOV R1, #0x3F800000
    // 修改内存保护
    Memory.protect(addr1, 0x1000, 'rwx');
    Memory.writeByteArray(addr1, arr1);
    var offset2 = 0x5E2A38;
    var addr2 = libil2cpp.base.add(offset2);
    var arr2 = [0x01, 0x11, 0xA0, 0xE3]; // MOV R1, #0x40000000
    // 修改内存保护
    Memory.protect(addr1, 0x1000, 'rwx');
    Memory.writeByteArray(addr1, arr1);
    // 将tiltSmooth修改为maxTiltSmooth的分支恢复
    var offset3 = 0x5E2A14; 
    var addr3 = libil2cpp.base.add(offset3);
    var arr3 = [0x0A, 0x00, 0x00, 0x5A]; // BPL 0x5E2A44
    // 修改内存保护
    Memory.protect(addr3, 0x1000, 'rwx');
    Memory.writeByteArray(addr3, arr3);
    // 恢复thrust推力
    // 由于不能直接拿到PlayerController的实例,需要先hook Update, 获得实例之后才能继续修改
    var pc_instance = NULL;
    var func_offset = 0x5E23E4; // PlayerController__Update
    var func_addr = libil2cpp.base.add(func_offset);
    // 函数对象
    var targetFunc = new NativeFunction(func_addr, 'void', ['pointer']);
    // hook
    Interceptor.attach(targetFunc, {
        onEnter: function (args) {
            // send(args[0]);
            pc_instance = args[0];
            Interceptor.detachAll();
            send(`PlayerController Instance: ${pc_instance}`);
            var thrust_ptr = pc_instance.add(0xC); // 偏移0xC
            Memory.writeFloat(thrust_ptr, 225);
            var thrust = Memory.readFloat(thrust_ptr);
            send(`thrust: ${thrust}`);
        }
    });
});
"""

# 加分数
add_score_js = """
Java.perform(() => {
    var libil2cpp = Process.findModuleByName("libil2cpp.so");
    // 读取GameManager_TypeInfo
    var gm_info_addr = libil2cpp.base.add(0x7690F0);
    var gm_info = Memory.readPointer(gm_info_addr);
    var gm_static_filed_addr = gm_info.add(0x5C); // 结构体偏移0x5C
    var gm_static_filed = Memory.readPointer(gm_static_filed_addr);
    var gm_instance = Memory.readPointer(gm_static_filed); // 偏移0x00
    var gm_score_ptr = gm_instance.add(0x50); // 分数偏移0x50
    var gm_score = Memory.readS32(gm_score_ptr);
    Memory.writeS32(gm_score_ptr, gm_score+100); // 加100分
    gm_score = Memory.readS32(gm_score_ptr); // 修改分数
    send(`game score: ${gm_score}`);
});
"""

# hook
# hook_js = """
# Java.perform(() => {
#     var libil2cpp = Process.findModuleByName("libil2cpp.so");
#     // hook函数偏移
#     var func_offset = 0x5E09BC; // GameManager__UpdateScore
#     var func_addr = libil2cpp.base.add(func_offset);
#     // 函数参数
#     var func_args = ['pointer', 'pointer'];
#     // 目标函数对象
#     var targetFunc = new NativeFunction(func_addr, 'pointer', func_args);
#     // hook
#     Interceptor.attach(targetFunc, {
#         onEnter: function (args) {
#             send("Enter Function");
#             send(args[0]);
#             send(args[1]);
#         },
#         onLeave: function(retval) {
#             send("Leave Function");
#             send(retval);
#         }
#     });
# });
# """

def on_message(message, data):
    if message['type'] == 'send':
        print("[*] {0}".format(message['payload']))
    else:
        print(message)

def main():
    device = frida.get_usb_device()
    app = device.get_frontmost_application()
    process = device.attach(app.pid)

    while (True):
        jscode = ""
        print("[0] : 退出")
        print("[1] : 开启无敌")
        print("[2] : 关闭无敌")
        print("[3] : 开启飞行")
        print("[4] : 关闭飞行")
        print("[5] : 加分数")
        # print("[9] : hook测试")
        choice = int(input(">> "))
        match choice:
            case 0:
                break
            case 1:
                jscode = invincible_js
            case 2:
                jscode = off_invincible_js
            case 3:
                jscode = fly_js
            case 4:
                jscode = off_fly_js
            case 5:
                jscode = add_score_js
            # case 9:
            #     jscode = hook_js
            case _:
                print("输入错误")
        script = process.create_script(jscode)
        script.on('message', on_message)
        script.load()

    process.detach()

if __name__ == "__main__":
    main()