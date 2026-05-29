# Mask My Scene

隐藏 Scene 守护进程端口与文件路径，屏蔽检测工具。

## 效果

| 检测手段 | 模块加载后 |
|----------|-----------|
| TCP `connect(127.0.0.1:PORT)` | → 端口被改写为 1，连接失败 |
| TCP/UDP `bind(PORT)` | → 端口被清零，内核分配随机端口 |
| `getsockname`（bind 后查询实际端口） | → 返回原始隐藏端口，应用无感知 |
| `/proc/net/tcp` / `/proc/net/tcp6` | → 隐藏端口被过滤 |
| `/proc/net/udp` / `/proc/net/udp6` | → 同上 |
| `stat` / `access` / `open` 扫描 Scene 路径 | → 路径不存在 |
| 白名单 UID 的进程 | → 正常，不受影响 |

---

## 前置条件

- 设备已 root（Magisk / KernelSU / APatch）
- 内核版本包含 `android` 标识及 KMI 版本号（如 `android15-6.6`）
- 对应 KMI 在以下范围内：`5.10`、`5.15`、`6.1`、`6.6`、`6.12`
- 内核已开启 `CONFIG_KPROBES=y`（通用 Android 内核默认开启）

---

## 安装

```bash
# 确认内核版本包含 android+KMI（如 android15-6.6）
uname -r
# 输出示例：6.6.30-android15-8-g3c7a8b9c0d1e
```

1. 前往 [Actions 页面](https://github.com/GirlandDragon/MaskMyScene/actions) 下载最新 `MaskMyScene-*.zip`
2. Magisk / KernelSU / APatch 中刷入

---

## 配置文件

路径：`/data/adb/MaskMyScene/config.txt`

安装时自动生成的默认配置：

```
# MaskMyScene configuration
ports=8765,8788,14731,14754
whitelist_uids=0,1000,2000
hooks=127
MinPath=2
```

> ⚠ 所有配置更改后需重启生效，不支持运行时热加载。

### `ports`

必填。要隐藏的端口号列表，多个用逗号分隔。

示例：
- `ports=8080`
- `ports=22,3306,8080`

### `whitelist_uids`

必填。白名单 UID 列表（必须包含 root 的 `0`）。白名单内的进程不受任何拦截。

示例中的 `10123` 仅为示意，实际 UID 以你设备为准。
安装过程会提示是否把 Scene 加入白名单，按音量键选择即可。

### `hooks`

功能开关位掩码。将所需功能的值相加：

| 值 | 功能 |
|----|------|
| 1 | connect 拦截 |
| 2 | bind 拦截 |
| 4 | getsockname 修正 |
| 8 | close 清理 |
| 16 | TCP `/proc/net` 过滤 |
| 32 | UDP `/proc/net` 过滤 |
| 64 | 路径隐藏 |

默认 `hooks=127`（1+2+4+8+16+32+64，全部开启）。

### `MinPath`

路径隐藏的激活阈值。整数 `0`~`4`。

模块加载后会扫描 Scene 相关路径，当找到的路径**达到或超过** `MinPath` 值时，路径隐藏才会激活。不满足则每 5 秒重试。

默认 `MinPath=2`。`MinPath=0` 为彻底关闭路径隐藏。

### `debug`

可选。设置 `debug=1` 后模块会输出更详细的调试日志（通过 `pr_debug`），可用于排查问题。默认未启用。

---

## 验证是否生效

加载模块后 dmesg 应有类似输出：

```
port_hider: hiding ports: 8765 8788 14731 14754
port_hider: vfs: blocked /dev/scene
port_hider: vfs: 2 paths >= MinPath=2, active
port_hider: loaded (v1.0.0)
```

查看 `lsmod | grep port_hider` 确认模块已加载。
