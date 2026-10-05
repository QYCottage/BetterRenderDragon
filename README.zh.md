<p align="right">
  <a href="./README.md">
    <img src="https://img.shields.io/badge/Switch%20to-English%20Documentation-blue?logo=googletranslate&labelColor=3e3e3e" alt="Switch to English" />
  </a>
</p>

<div align="center">

# BetterRenderDragon

**为渲染龙（RenderDragon）添加更多功能！适用于 Minecraft: Bedrock Edition（当前最新支持版本：26.4x）**

[![Release](https://img.shields.io/github/v/release/QYCottage/BetterRenderDragon?label=Release)](https://github.com/QYCottage/BetterRenderDragon/releases/latest)
![Total Downloads](https://img.shields.io/github/downloads/QYCottage/BetterRenderDragon/total?label=Total%20Downloads)
![Latest Release Downloads](https://img.shields.io/github/downloads/QYCottage/BetterRenderDragon/latest/total?label=Latest%20Downloads)
![Stars](https://img.shields.io/github/stars/QYCottage/BetterRenderDragon?label=Stars)
![Forks](https://img.shields.io/github/forks/QYCottage/BetterRenderDragon?label=Forks)
![License](https://img.shields.io/github/license/QYCottage/BetterRenderDragon?label=License)
[![Discord](https://img.shields.io/discord/1124235469245382698?logo=discord&label=Discord)](https://discord.gg/8nGcV8QkKZ)

</div>

## 功能特点

- **启用延迟渲染**
- **禁用光线追踪以获得更好的性能**
- **从资源包加载渲染龙着色器**
- **热重载渲染龙着色器**
- **可配置功能的 ImGui**

## 支持的 Minecraft 版本

| 类型   | 版本范围       |
| ------ | -------------- |
| 正式版 | 最新支持 26.4x |

## 使用方法

### 推荐方式（使用 MC 版本启动器）

1. **下载并安装 [QYCottage/mc-w10-version-launcher](https://github.com/QYCottage/mc-w10-version-launcher)**
   - 使用启动器选择并下载支持的 MCBE 版本（当前最新支持 26.4x）。

2. **下载并解压 [BetterRenderDragon.zip](#)**
   - 从 [Releases](https://github.com/QYCottage/BetterRenderDragon/releases/latest) 页面下载。

3. **复制插件文件**
   - 打开你下载的 MCBE 游戏目录。
   - 进入 `mods` 文件夹（如无请自行新建）。
   - 把 `BetterRenderDragon.dll` 复制到该文件夹。

4. **启动游戏并体验！**

### 备选方式（直接启动官方 MC）

1. **安装 Minecraft BE**
   - 版本要求：**当前最新支持 26.4x**

2. **下载并解压 [BetterRenderDragon.zip](#)**
   - 从 [Releases](https://github.com/QYCottage/BetterRenderDragon/releases/latest) 页面下载。

3. **启动 MC**
   - 正式版：双击 `LaunchMinecraft.bat`
   - 预览版：双击 `LaunchMinecraftPreview.bat`

> **建议优先使用版本启动器管理多版本（推荐方式）！**

## ImGui 快捷键

- 按 `F6` 键开启或关闭 ImGui 显示

## 最低要求

| 功能     | 最低要求    |
| -------- | ----------- |
| 运行     | Direct3D 11 |
| 延迟渲染 | Direct3D 12 |

> **注**：Direct3D 11 环境下无法启用延迟渲染。请确保你的设备和驱动符合该要求。

## 常见问题

- **杀毒软件拦截了 BetterRenderDragon，怎么办？**
  > 请将 BetterRenderDragon 添加到杀毒软件的白名单即可。该软件没有有效数字签名，属正常现象。

- **找不到 BetterRenderDragon 菜单！**
  > 按下 `F6` 键开启或关闭 BetterRenderDragon 菜单。

- **报错："FindFirstFileW failed: error [00000002] 找不到指定文件。"**
  > 很可能是杀毒软件删除了 `dlls` 文件夹中的 BetterRenderDragon dll 文件。请将其加入白名单后重试。

## 特别感谢

- [原作者](https://github.com/ddf8196/BetterRenderDragon)
- [QYCottage/mc-w10-version-launcher](https://github.com/QYCottage/mc-w10-version-launcher)

## 联系与反馈

如遇问题或建议，欢迎 [发起 Issue](https://github.com/QYCottage/BetterRenderDragon/issues) 或在讨论区留言！

## Discord 社区

加入我们的 Discord 社区获取技术支持和交流！

[![Discord Banner](./.github/assets/discord_banner.png)](https://discord.gg/8nGcV8QkKZ)

---

## 注意

**本程序与 Mojang Studios 没有任何隶属、合作、关联关系。**
