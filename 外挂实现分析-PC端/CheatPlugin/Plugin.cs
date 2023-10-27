using System;
using System.Reflection;
using BepInEx;
using BepInEx.Configuration;
using UnityEngine;
using HarmonyLib;

namespace CheatPlugin
{
    [BepInPlugin("noxke.plugins.CheatPlugin", "CheatPlugin", "1.0")]
    public class Plugin : BaseUnityPlugin
    {
        private void Awake()
        {
            // 插件启用时调用
            Logger.LogInfo($"Plugin {PluginInfo.PLUGIN_NAME} is loaded!");
            // 创建配置项
            GlobalVariables.invincible = Config.Bind("Config", "invincible", false, "碰撞无敌");
            GlobalVariables.invincible.Value = false;
            GlobalVariables.collision = Config.Bind("Config", "collision", true, "开启碰撞");
            GlobalVariables.collision.Value = true;
            GlobalVariables.fly = Config.Bind("Config", "fly", false, "开启飞行");
            GlobalVariables.fly.Value = false;
            GlobalVariables.score = Config.Bind("Config", "score", 0, "游戏分数");
            GlobalVariables.score.Value = 0;
            GlobalVariables.speed = Config.Bind("Config", "speed", 3f, "移动速度");
            GlobalVariables.speed.Value = 3f;
            // 关闭碰撞的回调函数
            GlobalVariables.collision.SettingChanged += (sender, args) => Cheat.instance.SetCollision(GlobalVariables.collision.Value);
            // 飞行回调函数
            GlobalVariables.fly.SettingChanged += (sender, args) => Cheat.instance.SetFly(GlobalVariables.fly.Value);
            // 修改分数的回调函数
            GlobalVariables.score.SettingChanged += (sender, args) => Cheat.instance.SetScore(GlobalVariables.score.Value);
            // 修改游戏速度的回调函数
            GlobalVariables.speed.SettingChanged += (sender, args) => Cheat.instance.SetSpeed(GlobalVariables.speed.Value);
        }
        private void Start()
        {
            // 所有插件全部加载后调用
            Logger.LogInfo($"Plugin {PluginInfo.PLUGIN_NAME} is started!");
            Cheat.instance = new Cheat();
            Cheat.instance.Start();
        }
        private void Update()
        {
            // 持续执行
            // Debug.Log($"Plugin {PluginInfo.PLUGIN_NAME} is Update!");
            Cheat.instance.Update();
        }
        private void OnDestroy()
        {
            // 插件关闭时调用
            Logger.LogInfo($"Plugin {PluginInfo.PLUGIN_NAME} is Destroyed!");
        }

        private void OnGUI()
        {
            // 在游戏界面上显示标签
            GUI.skin.label.fontSize = 18;
            GUI.skin.label.normal.textColor = Color.blue;
            string labelText = "FlappyBird Cheat Plugin\n";
            labelText += "written by noxke\n";
            labelText += "[F1] : config and more feature\n";
            labelText += "[ESC] : pause\n";
            labelText += $"[1/2] invincible [{GlobalVariables.invincible.Value}]\n";
            labelText += $"[3/4] : collision [{GlobalVariables.collision.Value}]\n";
            labelText += $"[5/6] : fly [{GlobalVariables.fly.Value}]\n";
            labelText += "[9] : add score\n";
            labelText += "[up/down/left/right] : move\n";
            labelText += "[0] : finish game\n";
            labelText += $"speed [{GlobalVariables.speed.Value}]";
            GUI.Label(new Rect(10, 10, 400, 300), labelText);
        }
    }
}
