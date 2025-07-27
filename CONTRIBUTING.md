# Contributing to Pipeline Guardian

Thank you for your interest in contributing to Pipeline Guardian! This document provides guidelines and information for contributors.

## 🤝 How to Contribute

### Types of Contributions

We welcome various types of contributions:

- **🐛 Bug Reports**: Help us identify and fix issues
- **💡 Feature Requests**: Suggest new analysis rules or improvements
- **📝 Documentation**: Improve guides, examples, and code comments
- **🔧 Code Contributions**: Implement new features or fixes
- **🎨 UI/UX**: Help improve the user interface and experience
- **🧪 Testing**: Test on different projects and asset types
- **🌐 Localization**: Help translate the plugin to other languages

## 🚀 Getting Started

### Prerequisites

- Unreal Engine 5.5 (source or binary)
- Visual Studio 2019/2022
- Windows 10/11 (64-bit)
- Git

### Development Setup

1. **Fork the Repository**
   ```bash
   git clone https://github.com/yourusername/PipelineGuardian.git
   cd PipelineGuardian
   ```

2. **Set Up Your Development Environment**
   - Ensure Unreal Engine 5.5 is properly installed
   - Install Visual Studio with C++ development tools
   - Set up your UE5 environment variables

3. **Build the Plugin**
   ```bash
   # Generate project files
   UnrealBuildTool PipelineGuardian.uplugin
   
   # Open in Visual Studio
   PipelineGuardian.sln
   ```

4. **Test Your Setup**
   - Build the plugin in Visual Studio
   - Copy to a test Unreal project
   - Verify the plugin loads correctly

## 📋 Development Guidelines

### Code Standards

#### Unreal Engine 5.5 Compliance
- Follow Epic Games' C++ Coding Standard
- Use UE5.5-specific APIs and patterns
- Ensure compatibility with UE5.5 features

#### C++ Standards
- Use C++17/20 features appropriately
- Follow modern C++ best practices
- Use RAII and smart pointers
- Prefer `const` correctness

#### Naming Conventions
- **Classes**: PascalCase (e.g., `FMyClass`, `UMyObject`)
- **Functions**: PascalCase (e.g., `MyFunction()`)
- **Variables**: PascalCase (e.g., `MyVariable`)
- **Constants**: ALL_CAPS (e.g., `MAX_COUNT`)
- **Files**: PascalCase (e.g., `MyClass.h`, `MyClass.cpp`)

#### Unreal Engine Specifics
- Use `UPROPERTY()`, `UFUNCTION()`, `UCLASS()` macros appropriately
- Use `TEXT()` macro for string literals
- Use `UE_LOG` for logging
- Use Unreal container classes (`TArray`, `TMap`, etc.)

### Architecture Guidelines

#### Adding New Analysis Rules

1. **Create Rule Header** (`Private/Analysis/Rules/[AssetType]/F[AssetType][RuleName]Rule.h`)
   ```cpp
   #pragma once
   
   #include "CoreMinimal.h"
   #include "Analysis/IAssetCheckRule.h"
   
   class FStaticMeshExampleRule : public IAssetCheckRule
   {
   public:
       virtual bool Check(UObject* AssetObject, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
       virtual FName GetRuleID() const override;
       virtual FText GetRuleDescription() const override;
   };
   ```

2. **Implement Rule** (`Private/Analysis/Rules/[AssetType]/F[AssetType][RuleName]Rule.cpp`)
   ```cpp
   #include "FStaticMeshExampleRule.h"
   // ... implementation
   ```

3. **Register Rule** in the appropriate analyzer
4. **Add Settings** to `FPipelineGuardianSettings.h`
5. **Update Documentation**

#### Adding New Asset Analyzers

1. **Create Analyzer Header** (`Private/Analysis/AssetTypeAnalyzers/F[AssetType]Analyzer.h`)
2. **Implement Analyzer** (`Private/Analysis/AssetTypeAnalyzers/F[AssetType]Analyzer.cpp`)
3. **Register Analyzer** in `FAssetScanner`
4. **Add UI Support** if needed

### Testing Guidelines

#### Unit Testing
- Write unit tests for new analysis rules
- Test edge cases and error conditions
- Ensure tests pass on different UE5.5 versions

#### Integration Testing
- Test with various asset types and sizes
- Verify UI behavior and responsiveness
- Test configuration persistence

#### Performance Testing
- Profile analysis performance on large asset sets
- Ensure async operations don't block the editor
- Monitor memory usage

## 📝 Pull Request Process

### Before Submitting

1. **Check Existing Issues**: Ensure your PR addresses an existing issue or adds significant value
2. **Test Thoroughly**: Test your changes on different projects and asset types
3. **Update Documentation**: Add or update relevant documentation
4. **Follow Style Guide**: Ensure code follows our style guidelines

### PR Template

Use this template when creating pull requests:

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] UI/UX improvement
- [ ] Performance optimization
- [ ] Other (please describe)

## Testing
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Tested on different UE5.5 versions
- [ ] Performance impact assessed

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No breaking changes introduced
- [ ] Backward compatibility maintained

## Screenshots (if applicable)
Add screenshots for UI changes

## Additional Notes
Any additional information
```

### Review Process

1. **Automated Checks**: CI/CD will run automated tests
2. **Code Review**: At least one maintainer will review your code
3. **Testing**: Your changes will be tested on various setups
4. **Approval**: Once approved, your PR will be merged

## 🐛 Bug Reports

### Before Reporting

1. **Check Existing Issues**: Search for similar issues
2. **Update to Latest Version**: Ensure you're using the latest release
3. **Test in Clean Project**: Verify the issue isn't project-specific

### Bug Report Template

```markdown
## Bug Description
Clear description of the issue

## Steps to Reproduce
1. Step 1
2. Step 2
3. Step 3

## Expected Behavior
What should happen

## Actual Behavior
What actually happens

## Environment
- Unreal Engine Version: 5.5.x
- Plugin Version: 1.0.x
- OS: Windows 10/11
- Graphics Card: (if relevant)

## Additional Information
- Screenshots/videos
- Log files
- Sample assets (if applicable)
```

## 💡 Feature Requests

### Before Requesting

1. **Check Roadmap**: See if it's already planned
2. **Search Issues**: Check for existing requests
3. **Consider Impact**: Think about implementation complexity

### Feature Request Template

```markdown
## Feature Description
Clear description of the requested feature

## Use Case
Why this feature would be useful

## Proposed Implementation
How you think it could be implemented (optional)

## Alternatives Considered
Other approaches you've considered (optional)

## Additional Context
Any other relevant information
```

## 🎨 UI/UX Contributions

### Design Principles

- **Consistency**: Follow Unreal Editor design patterns
- **Accessibility**: Ensure features are accessible to all users
- **Performance**: UI should remain responsive during analysis
- **Clarity**: Information should be clear and well-organized

### UI Guidelines

- Use Slate widgets appropriately
- Follow Unreal Editor color schemes
- Provide clear feedback for user actions
- Include tooltips for complex features

## 📚 Documentation

### Code Documentation

- Use Doxygen-style comments for public APIs
- Document complex algorithms and business logic
- Include examples for new features
- Update README for significant changes

### User Documentation

- Write clear, step-by-step guides
- Include screenshots for UI features
- Provide configuration examples
- Document known limitations

## 🔧 Development Tools

### Recommended Tools

- **Visual Studio 2022**: Primary IDE
- **Unreal Engine 5.5**: For testing and debugging
- **Git**: Version control
- **Visual Studio Code**: For quick edits (optional)

### Useful Extensions

- **Unreal Engine 5.5 Documentation**: For API reference
- **C++ IntelliSense**: For better code completion
- **GitLens**: For enhanced Git integration

## 🏷️ Version Control

### Branch Strategy

- **main**: Stable, production-ready code
- **develop**: Integration branch for features
- **feature/***: Individual feature branches
- **bugfix/***: Bug fix branches
- **hotfix/***: Critical fixes for production

### Commit Messages

Use conventional commit format:

```
type(scope): description

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Maintenance tasks

## 🎯 Contribution Areas

### High Priority

- **Material Analyzer**: Implement material analysis rules
- **Texture Analyzer**: Add texture validation features
- **Performance Optimization**: Improve analysis speed
- **Bug Fixes**: Address reported issues

### Medium Priority

- **Blueprint Analyzer**: Add Blueprint validation
- **UI Improvements**: Enhance user experience
- **Documentation**: Improve guides and examples
- **Testing**: Add comprehensive test coverage

### Low Priority

- **Localization**: Add support for other languages
- **Advanced Features**: Complex analysis features
- **Integration**: Third-party tool integration

## 📞 Getting Help

### Questions and Support

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For questions and general discussion
- **Discord**: For real-time chat and support
- **Email**: For private or sensitive matters

### Resources

- [Unreal Engine 5.5 Documentation](https://docs.unrealengine.com/5.5/)
- [Epic Games C++ Coding Standard](https://docs.unrealengine.com/5.5/en-US/epic-cplusplus-coding-standard-for-unreal-engine/)
- [Slate UI Framework](https://docs.unrealengine.com/5.5/en-US/slate-ui-framework-in-unreal-engine/)

## 🙏 Recognition

### Contributors

All contributors will be recognized in:
- README.md contributors section
- Plugin credits
- Release notes
- GitHub contributors page

### Special Recognition

- **Major Contributors**: Those who make significant contributions
- **Long-term Contributors**: Those who contribute consistently
- **Community Leaders**: Those who help others and promote the project

---

Thank you for contributing to Pipeline Guardian! Your help makes the Unreal Engine community better for everyone. 