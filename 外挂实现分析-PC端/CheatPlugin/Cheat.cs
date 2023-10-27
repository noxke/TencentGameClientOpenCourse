using System;
using System.Reflection;
using BepInEx;
using UnityEngine;
using HarmonyLib;

namespace CheatPlugin
{
    public class Cheat
    {
        // 核心实现
        public void Start()
        {
            SetPatch(); // 设置Patch
        }
        public void Update()
        {
            // 循环调用
            KeyBound(); // 按键检测
            GetObjects(); // 获取游戏对象
            if (GlobalVariables.birdInstances != null)
            {
                GlobalVariables.score.Value = GlobalVariables.birdInstances.score; // 同步分数
            }
        }

        private void KeyBound()
        {
            // 按键绑定
            if (Input.GetKey(KeyCode.Escape))
            {
                // ESC暂停 鼠标太难点了
                if (GamePlayController.instance != null)
                {
                    Debug.Log("key ESC down, pause");
                    GamePlayController.instance.pauseGame();
                }
            }
            if (Input.GetKey(KeyCode.Alpha0))
            {
                // 0游戏胜利
                FinishGame();
            }
            if (Input.GetKey(KeyCode.Alpha1))
            {
                // 1开启无敌
                Debug.Log("key 1 down, invincible on");
                GlobalVariables.invincible.Value = true;
            }
            if (Input.GetKey(KeyCode.Alpha2))
            {
                // 2关闭无敌
                Debug.Log("key 2 down, invincible off");
                GlobalVariables.invincible.Value = false;
            }
            if (Input.GetKey(KeyCode.Alpha3))
            {
                // 3开启碰撞
                Debug.Log("key 3 down, collision on");
                GlobalVariables.collision.Value = true;
            }
            if (Input.GetKey(KeyCode.Alpha4))
            {
                // 4关闭碰撞
                Debug.Log("key 4 down, collision off");
                GlobalVariables.collision.Value = false;
            }
            if (Input.GetKey(KeyCode.Alpha5))
            {
                // 5开启飞行
                Debug.Log("key 5 down, fly on");
                GlobalVariables.fly.Value = true;
            }
            if (Input.GetKey(KeyCode.Alpha6))
            {
                // 6关闭飞行
                Debug.Log("key 6 down, fly off");
                GlobalVariables.fly.Value = false;
            }
            if (Input.GetKey(KeyCode.Alpha9))
            {
                // 9加分
                GlobalVariables.score.Value += 1;
                SetScore(GlobalVariables.score.Value);
            }
            if (Input.GetKey(KeyCode.UpArrow))
            {
                // 上键向上飞
                BirdMove(0f, 0.1f);
            }
            if (Input.GetKey(KeyCode.DownArrow))
            {
                // 下键向下飞
                BirdMove(0f, -0.1f);
            }
            if (Input.GetKey(KeyCode.LeftArrow))
            {
                // 左键向左飞
                BirdMove(-0.1f, 0f);
            }
            if (Input.GetKey(KeyCode.RightArrow))
            {
                // 右键向右飞
                BirdMove(0.1f, 0f);
            }
        }

        public void GetObjects()
        {
            // 获取游戏内对象和实例
            GlobalVariables.birdInstances = BirdScripts.instance; // 获取当前小鸟实例
            if (GlobalVariables.birdInstances != null)
            {
                // 获取小鸟的组件
                GlobalVariables.birdCollider2D = GlobalVariables.birdInstances.GetComponent<Collider2D>();
                GlobalVariables.birdRigidbody2D = GlobalVariables.birdInstances.GetComponent<Rigidbody2D>();
            }
            GlobalVariables.pipes = GameObject.FindGameObjectsWithTag("Pipe");  // 管道
            GlobalVariables.pipeHolders = GameObject.FindGameObjectsWithTag("PipeHolder"); // 获取当前的管道间隙
            GlobalVariables.enemies = GameObject.FindGameObjectsWithTag("Enemy");  // 敌人
            GlobalVariables.flags = GameObject.FindGameObjectsWithTag("Flag");  // flag
        }
        public void SetPatch()
        // 设置勾取函数
        {
            // patch OnCollisionEnter2D方法
            Harmony harmony1 = new Harmony("com.noxke.patch1");
            MethodInfo onCollisionEnter2D = AccessTools.Method(typeof(BirdScripts), "OnCollisionEnter2D");
            harmony1.Patch(onCollisionEnter2D, prefix: new HarmonyMethod(typeof(Cheat).GetMethod("PrefixOnCollisionEnter2D")));
            Debug.Log("Patch OnCollisionEnter2D");

            // patch flapTheBird方法
            Harmony harmony2 = new Harmony("com.noxke.patch2");
            MethodInfo flapTheBird = AccessTools.Method(typeof(BirdScripts), "flapTheBird");
            harmony2.Patch(flapTheBird, prefix: new HarmonyMethod(typeof(Cheat).GetMethod("PrefixFlapTheBird")));
            Debug.Log("Patch flapTheBird");
        }
        public static bool PrefixOnCollisionEnter2D(BirdScripts __instance, Collision2D target)
        {
            // 在发生碰撞前拦截
            if (GlobalVariables.invincible.Value != true)
            {
                return true;
            }
            // 碰撞到flag的时候胜利，不做拦截
            if (target.gameObject.tag == "Pipe" || target.gameObject.tag == "Ground" || target.gameObject.tag == "Enemy")
            {
                Debug.Log($"Collision {target.gameObject.tag}");
                return false;
            }
            return true;
        }

        public static bool PrefixFlapTheBird(BirdScripts __instance)
        {
            // 在flap前调用
            // 避免飞行时flap然后无法减速
            Debug.Log("Patch flapTheBird");
            if (GlobalVariables.fly.Value == true)
            {
                return false;
            }
            return true;
        }

        public void SetCollision(bool value)
        {
            // 设置管道和敌人的碰撞
            GlobalVariables.collision.Value = value;
            for (int i = 0; i < GlobalVariables.pipes.Length; i++)
            {
                BoxCollider2D pipeBoxCollider2D = GlobalVariables.pipes[i].GetComponent<BoxCollider2D>();
                if (pipeBoxCollider2D != null)
                {
                    pipeBoxCollider2D.enabled = value;
                }
            }
            for (int i = 0; i < GlobalVariables.enemies.Length; i++)
            {
                BoxCollider2D enemyBoxCollider2D = GlobalVariables.enemies[i].GetComponent<BoxCollider2D>();
                if (enemyBoxCollider2D == null)
                {
                    Collider2D enemyCollider2D = GlobalVariables.enemies[i].GetComponent<Collider2D>();
                    if (enemyCollider2D != null)
                    {
                        enemyCollider2D.enabled = value;
                    }
                }
                else
                {
                    enemyBoxCollider2D.enabled = value;
                }
            }
            Debug.Log($"collision : {value}");
        }

        public void SetFly(bool value)
        {
            if (value == true)
            {
                // 关闭重力影响
                GlobalVariables.birdRigidbody2D.gravityScale = 0f;
                // 速度向量设为0
                GlobalVariables.birdRigidbody2D.velocity = new Vector2(0f, 0f);
            }
            else
            {
                // 恢复重力影响
                GlobalVariables.birdRigidbody2D.gravityScale = 1f;
            }
            Debug.Log($"fly : {value}");
        }

        public void SetScore(int value)
        {
            // 设置游戏分数
            GlobalVariables.birdInstances.score = value;
            GamePlayController.instance.setScore(value);
            Debug.Log($"set score  {value}");
        }

        public void SetSpeed(float value)
        {
            // 设置x轴速度
            GlobalVariables.speed.Value = value;
            // 私有成员需要使用反射修改
            FieldInfo privateField = typeof(BirdScripts).GetField("forwardSpeed", BindingFlags.NonPublic | BindingFlags.Instance);
            if (privateField != null)
            {
                privateField.SetValue(GlobalVariables.birdInstances, value);
                Debug.Log($"forward speed : {value}");
            }
        }

        public void BirdMove(float x, float y)
        {
            if (GlobalVariables.birdInstances != null)
            {
                Vector3 basePosition = GlobalVariables.birdInstances.transform.position;
                basePosition.x += x;
                basePosition.y += y;
                GlobalVariables.birdInstances.transform.position = basePosition;
                Debug.Log($"move bird x : {x}, y : {y}");
            }
        }

        public void FinishGame()
        {
            // 游戏胜利
            if (GlobalVariables.birdInstances != null)
            {
                GlobalVariables.birdInstances.isAlive = false;
                // 需要利用反射调用私有方法和获取私有变量
                FieldInfo cheerClipFiled = typeof(BirdScripts).GetField("cheerClip", BindingFlags.NonPublic | BindingFlags.Instance);
                AudioSource audioSource = GlobalVariables.birdInstances.GetComponent<AudioSource>();
                audioSource.PlayOneShot((AudioClip)cheerClipFiled.GetValue(GlobalVariables.birdInstances));
                GamePlayController.instance.finishGame();
                Debug.Log("finish game!");
            }
        }

        public static Cheat instance;
    }
}