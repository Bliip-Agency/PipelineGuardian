# Pipeline Guardian

[![Unreal Engine 5.5](https://img.shields.io/badge/Unreal%20Engine-5.5-blue.svg)](https://www.unrealengine.com/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0-orange.svg)](PipelineGuardian.uplugin)

A comprehensive Unreal Engine 5.5 editor plugin for deep asset analysis and pipeline integrity maintenance. Pipeline Guardian automatically scans and validates your assets against configurable quality standards, helping maintain consistent asset quality across your project.

## 🎯 Overview

Pipeline Guardian is designed to catch common asset issues before they impact your game's performance, visual quality, or development workflow. It provides automated analysis, detailed reporting, and in many cases, automatic fixes for detected issues.

### Key Features

- **🔍 Comprehensive Asset Analysis**: Deep scanning of Static Mesh assets with 15+ quality checks
- **⚙️ Configurable Rules**: Extensive settings system with profile-based configuration
- **🚀 Async Processing**: Non-blocking analysis for large asset sets
- **🔧 Auto-Fix Capabilities**: Automatic correction for many common issues
- **📊 Detailed Reporting**: Rich UI with filtering, sorting, and export capabilities
- **🎨 Modern UI**: Clean, intuitive Slate-based interface integrated into Unreal Editor

## 📋 Current Features

### Static Mesh Analysis (15 Rules)

| Rule Category | Description | Auto-Fix |
|---------------|-------------|----------|
| **Naming Convention** | Validates asset naming patterns using wildcards | ✅ |
| **LOD Management** | Detects missing LODs and insufficient polygon reduction | ✅ |
| **Lightmap UV** | Identifies missing/invalid lightmap UV configuration | ✅ |
| **UV Overlapping** | Detects overlapping UV coordinates across multiple channels | ❌ |
| **Triangle Count** | Performance impact analysis with configurable thresholds | ❌ |
| **Degenerate Faces** | Zero-area triangle detection | ❌ |
| **Collision** | Missing collision geometry and complexity analysis | ✅ |
| **Nanite Suitability** | Automatic Nanite enable/disable recommendations | ✅ |
| **Material Slots** | Slot count validation and empty slot management | ✅ |
| **Vertex Colors** | Missing vertex color detection and channel validation | ❌ |
| **Transform/Pivot** | Pivot position and scaling analysis | ❌ |
| **Lightmap Resolution** | Resolution validation with auto-optimization | ✅ |
| **Socket Naming** | Socket naming conventions and positioning | ✅ |
| **Scaling** | Non-uniform scale and zero-scale detection | ✅ |
| **Transform Rules** | Asset transform validation | ❌ |

### Analysis Modes

- **Project-Wide Scan**: Analyze all assets in your project
- **Selected Folders**: Scan specific content directories
- **Selected Assets**: Analyze currently selected assets in Content Browser
- **Open Level**: Analyze assets referenced in the current level

### Configuration System

- **Profile-Based Settings**: Create and switch between different analysis profiles
- **JSON Import/Export**: Share configurations across teams
- **Granular Control**: Enable/disable individual rules and set custom thresholds
- **Severity Levels**: Configure Critical, Error, Warning, and Info levels per rule

## 🚀 Installation

### Prerequisites

- Unreal Engine 5.5
- Windows 10/11 (64-bit)

### Installation Steps

1. **Download the Plugin**
   ```bash
   git clone https://github.com/yourusername/PipelineGuardian.git
   ```

2. **Copy to Your Project**
   - Copy the `PipelineGuardian` folder to your project's `Plugins/` directory
   - Or copy to `Engine/Plugins/` for engine-wide installation

3. **Enable the Plugin**
   - Open your Unreal project
   - Go to **Edit > Plugins**
   - Search for "Pipeline Guardian"
   - Enable the plugin and restart the editor

4. **Access the Tool**
   - Navigate to **Window > Pipeline Guardian** in the main menu
   - Or use the toolbar button (if configured)

## 📖 Usage

### Quick Start

1. **Open Pipeline Guardian**
   - Go to **Window > Pipeline Guardian**

2. **Configure Settings** (Optional)
   - Click **Settings** to customize analysis rules
   - Create or load a profile for your project's standards

3. **Run Analysis**
   - Choose your analysis mode (Project, Selected, etc.)
   - Click **Analyze** to start the scan

4. **Review Results**
   - Browse through detected issues
   - Use filters to focus on specific problems
   - Apply auto-fixes where available

### Configuration

#### Creating a Profile

1. Open **Project Settings > Pipeline Guardian**
2. Configure your desired rules and thresholds
3. Save your settings as a profile

#### Customizing Rules

Each rule can be individually configured with:
- **Enable/Disable**: Turn rules on or off
- **Severity Levels**: Set Critical, Error, Warning, or Info
- **Thresholds**: Configure specific values for your project
- **Auto-Fix**: Enable automatic correction where available

## 🔧 Configuration Examples

### Static Mesh Naming Convention
```
Pattern: SM_*
Description: All static meshes must start with "SM_"
```

### LOD Requirements
```
Minimum LODs: 3
Reduction Percentage: 50%
Auto-Create Missing LODs: Enabled
```

### Performance Thresholds
```
Triangle Count Warning: 10,000
Triangle Count Error: 50,000
Nanite Enable Threshold: 5,000
```

## 🐛 Known Issues & Limitations

### Current Limitations
- **Single Asset Type**: Currently only supports Static Mesh analysis
- **Texture Analysis**: Framework exists but not yet implemented
- **Large Asset Sets**: Very large projects may experience performance impact
- The rules that don't have auto-fix are intentionally designed that way because they require external tools for best results (UV tools, mesh optimization tools, DCC tools).

### Known Issues
- Some auto-fix operations may require asset reimport
- UV overlap detection may be slow on complex meshes
- Profile switching may require editor restart in some cases

### Reporting Bugs

If you encounter issues, please:

1. **Check the Logs**: Look for `LogPipelineGuardian` entries in the Output Log
2. **Provide Details**: Include UE version, plugin version, and steps to reproduce
3. **Attach Assets**: If possible, provide sample assets that trigger the issue
4. **Use GitHub Issues**: Create a detailed issue report with the bug template

## 🛠️ Development

### Building from Source

1. **Clone the Repository**
   ```bash
   git clone https://github.com/yourusername/PipelineGuardian.git
   cd PipelineGuardian
   ```

2. **Build Requirements**
   - Unreal Engine 5.5 source code
   - Visual Studio 2019/2022
   - Windows 10/11 SDK

3. **Compile**
   ```bash
   # Generate project files
   UnrealBuildTool PipelineGuardian.uplugin
   
   # Build the plugin
   msbuild PipelineGuardian.sln
   ```

### Architecture Overview

```
PipelineGuardian/
├── Public/                    # Public headers
│   ├── Analysis/             # Analysis interfaces and results
│   ├── UI/                   # Slate widget headers
│   └── Core/                 # Core plugin headers
├── Private/                   # Implementation files
│   ├── Analysis/             # Rule implementations
│   │   ├── Rules/           # Individual check rules
│   │   └── AssetTypeAnalyzers/ # Asset type orchestrators
│   ├── Core/                 # Core functionality
│   └── UI/                   # Slate widget implementations
└── Resources/                 # Plugin resources
```

### Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

## 🗺️ Roadmap

### 🚧 Work in Progress

#### Material Analyzer
- Material complexity analysis
- Shader performance validation
- Texture reference validation
- Material slot consistency

#### Texture Analyzer
- Resolution validation
- Compression settings
- Mipmap generation
- Texture format optimization

#### Blueprint Analyzer
- Compilation error detection
- Performance issue identification
- Best practice validation
- Dependency analysis

#### Skeletal Mesh Analyzer
- Bone hierarchy validation
- LOD configuration
- Physics asset compatibility
- Animation compatibility

#### Animation Analyzer
- Keyframe optimization
- Compression settings
- Root motion validation
- Animation length standards

#### Niagara Analyzer
- Performance impact analysis
- GPU particle limits
- Emitter configuration
- Material compatibility

#### Sound Analyzer
- Audio format validation
- Compression settings
- Spatialization setup
- Performance optimization

#### Level Analyzer
- Asset reference validation
- Lighting setup analysis
- Performance profiling
- Optimization recommendations

#### Data Asset Analyzer
- Schema validation
- Reference integrity
- Performance impact
- Best practice compliance

### 🎨 UI/UX Updates

#### Enhanced Reporting & UI
- **Advanced Filtering**: Multi-criteria filtering and search
- **Export Capabilities**: PDF, CSV, and HTML report generation
- **Dashboard View**: Project health overview and metrics
- **Custom Themes**: Dark/light mode and customizable appearance

#### Quality of Life Updates
- **Preset Management**: Quick-switch between common configurations
- **Keyboard Shortcuts**: Hotkeys for common operations
- **Context Menus**: Right-click integration in Content Browser

### 🚀 Advanced Features & Polish

#### Performance Optimizations
- **Incremental Analysis**: Only scan changed assets
- **Background Processing**: Continuous monitoring mode
- **Caching System**: Cache analysis results for faster re-runs
- **Parallel Processing**: Multi-threaded analysis for large projects

#### Integration Features
- **CI/CD Integration**: Command-line interface for automated builds
- **Version Control**: Git integration for tracking asset changes
- **Team Collaboration**: Shared profiles and team standards
- **API Access**: Programmatic access for custom tools

## 🤝 Contributing

We welcome contributions from the community! Here's how you can help:

### Ways to Contribute

- **🐛 Bug Reports**: Help us identify and fix issues
- **💡 Feature Requests**: Suggest new analysis rules or improvements
- **📝 Documentation**: Improve guides and examples
- **🔧 Code Contributions**: Implement new features or fixes
- **🎨 UI/UX**: Help improve the user interface
- **🧪 Testing**: Test on different projects and asset types

### Getting Started

1. **Fork the Repository**
2. **Create a Feature Branch**: `git checkout -b feature/amazing-feature`
3. **Make Your Changes**: Follow our coding standards
4. **Test Thoroughly**: Ensure your changes work correctly
5. **Submit a Pull Request**: Use our PR template

### Development Guidelines

- Follow Unreal Engine 5.5 coding standards
- Add comprehensive documentation for new features
- Include unit tests for new analysis rules
- Ensure backward compatibility when possible
- Update the changelog for significant changes

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Epic Games**: For the amazing Unreal Engine platform
- **Unreal Community**: For inspiration and feedback
- **Contributors**: Everyone who has helped improve Pipeline Guardian

## 📞 Support

### Getting Help

- **📖 Documentation**: Check this README and inline code documentation
- **🐛 Issues**: Report bugs on [GitHub Issues] (https://github.com/Bliip-Agency/PipelineGuardian/issues)
- **💬 Discussions**: Join our [GitHub Discussions](https://github.com/Bliip-Agency/PipelineGuardian/discussions)
- **📧 Email**: Contact us at [bliipagency@gmail.com]

### Community

If you Like what I have made Please contribute over here to further enhance and update the plugin!
https://ko-fi.com/psiclone

---

**Made with ❤️ for the Unreal Engine community**

*Pipeline Guardian - Keeping your assets in check, one asset at a time.* 