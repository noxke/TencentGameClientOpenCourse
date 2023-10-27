using System;
using System.Reflection;
using BepInEx;
using BepInEx.Configuration;
using UnityEngine;
using HarmonyLib;

namespace CheatPlugin
{
    public static class GlobalVariables
    {
        // 全局变量
        public static BirdScripts birdInstances; // 当前小鸟实例
        public static Rigidbody2D birdRigidbody2D; // 小鸟的Rigidbody2D组件
        public static Collider2D birdCollider2D; // 小鸟的Collider2D组件
        public static GameObject[] pipes; // 管道
        public static GameObject[] pipeHolders; // 管道间隙
        public static GameObject[] enemies; // 敌人
        public static GameObject[] flags; // flag
        public static ConfigEntry<bool> invincible; // 无敌
        public static ConfigEntry<bool> collision; // 碰撞
        public static ConfigEntry<bool> fly; // 飞行
        public static ConfigEntry<float> speed; // 游戏速度
        public static ConfigEntry<int> score; // 分数
    }
}
