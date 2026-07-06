---
name: skill-installer
description: Install and manage Dioxus agent skills from the official repository. Use when setting up new development environments or adding specific Dioxus capabilities to existing projects.
---

# Dioxus Skill Installer

This skill helps you install and manage Dioxus agent skills for building applications with the Dioxus framework.

## When to use this skill

- Installing Dioxus skills for the first time
- Adding specific skills to existing projects
- Updating skills to latest versions
- Managing skill dependencies and configurations
- Setting up team development environments

## Available Dioxus Skills

### Core Skills
- **dioxus-project-setup**: Project initialization and configuration
- **dioxus-component-dev**: Component development and RSX patterns
- **dioxus-routing**: Client-side routing and navigation
- **dioxus-state-management**: State management with signals and stores
- **dioxus-fullstack-dev**: Fullstack development with server functions

### Installation Commands

**Install Individual Skills:**
```bash
# Install project setup skill
$skill-installer install dioxus-project-setup

# Install component development skill
$skill-installer install dioxus-component-dev

# Install routing skill
$skill-installer install dioxus-routing

# Install state management skill
$skill-installer install dioxus-state-management

# Install fullstack development skill
$skill-installer install dioxus-fullstack-dev
```

**Install All Dioxus Skills:**
```bash
$skill-installer install-dioxus-suite
```

**Install Skills by Category:**
```bash
# Install basic development skills
$skill-installer install-category basic

# Install advanced development skills
$skill-installer install-category advanced

# Install fullstack development skills
$skill-installer install-category fullstack
```

## Installation Process

### 1. Automatic Installation

The installer will:
1. Detect your current environment (project vs user vs system)
2. Download skills from the official repository
3. Install to appropriate location
4. Verify skill functionality
5. Update configuration files

### 2. Manual Installation Steps

If automatic installation fails:

```bash
# Create skills directory
mkdir -p ~/.agents/skills

# Clone the Dioxus skills repository
cd ~/.agents/skills
git clone https://github.com/dioxus-community/agent-skills.git dioxus

# Symlink individual skills
ln -s dioxus/.agents/skills/dioxus-project-setup ./
ln -s dioxus/.agents/skills/dioxus-component-dev ./
ln -s dioxus/.agents/skills/dioxus-routing ./
ln -s dioxus/.agents/skills/dioxus-state-management ./
ln -s dioxus/.agents/skills/dioxus-fullstack-dev ./
```

### 3. Project-Specific Installation

For project-specific skills:

```bash
# Navigate to your project
cd your-dioxus-project

# Create local skills directory
mkdir -p .agents/skills

# Install skills locally
$skill-installer install dioxus-project-setup --local
$skill-installer install dioxus-component-dev --local
```

## Skill Management

### Listing Installed Skills

```bash
# List all installed skills
$skill-installer list

# List Dioxus-specific skills
$skill-installer list --filter dioxus

# Show skill details
$skill-installer info dioxus-component-dev
```

### Updating Skills

```bash
# Update all skills
$skill-installer update

# Update specific skill
$skill-installer update dioxus-routing

# Update to specific version
$skill-installer update dioxus-fullstack-dev --version 1.2.0
```

### Removing Skills

```bash
# Remove specific skill
$skill-installer remove dioxus-routing

# Remove all Dioxus skills
$skill-installer remove-category dioxus

# Clean up unused skills
$skill-installer cleanup
```

## Configuration

### Skill Categories

**Basic Development (Recommended for beginners):**
- dioxus-project-setup
- dioxus-component-dev

**Intermediate Development:**
- dioxus-routing
- dioxus-state-management

**Advanced Development:**
- dioxus-fullstack-dev

### Installation Locations

1. **System Level** (`/etc/agents/skills/`):
   - Available to all users
   - Requires admin privileges
   - Good for shared development machines

2. **User Level** (`~/.agents/skills/`):
   - Available to current user only
   - No admin privileges required
   - Good for personal development

3. **Project Level** (`./agents/skills/`):
   - Available to current project only
   - Version controlled with project
   - Good for team consistency

### Dependencies

The installer will automatically handle:
- Rust toolchain verification
- Dioxus CLI installation
- Platform-specific requirements
- Skill interdependencies

## Prerequisites Check

The installer verifies these requirements:

```bash
# Check Rust installation
rustc --version  # Should be 1.70.0+

# Check Cargo installation
cargo --version

# Check Dioxus CLI (will install if missing)
dx --version

# Check platform-specific tools
# Web: wasm-pack
# Desktop: System WebView
# Mobile: Platform SDKs
```

## Troubleshooting

### Common Issues

**Skills not appearing:**
1. Restart your IDE/editor
2. Check skill directory permissions
3. Verify installation location

**Permission errors:**
```bash
# Fix permissions
chmod -R 755 ~/.agents/skills

# Or install to user directory
$skill-installer install dioxus-project-setup --user
```

**Network issues:**
```bash
# Use offline installation
$skill-installer install --offline /path/to/skills

# Configure proxy
export HTTP_PROXY=http://proxy.example.com:8080
$skill-installer install dioxus-component-dev
```

**Version conflicts:**
```bash
# Check installed versions
$skill-installer list --versions

# Force reinstall
$skill-installer install dioxus-routing --force

# Install specific version
$skill-installer install dioxus-fullstack-dev --version 1.0.0
```

## Advanced Usage

### Custom Skill Repositories

```bash
# Add custom repository
$skill-installer add-repo https://github.com/yourorg/custom-dioxus-skills.git

# Install from custom repo
$skill-installer install custom-skill --repo yourorg

# List available repositories
$skill-installer repo list
```

### Batch Installation

Create a `skills.yaml` configuration file:

```yaml
skills:
  - name: dioxus-project-setup
    version: latest
    location: user
  - name: dioxus-component-dev
    version: 1.0.0
    location: project
  - name: dioxus-routing
    version: latest
    location: user

categories:
  - basic
  - intermediate

settings:
  auto_update: false
  check_dependencies: true
```

Install from configuration:
```bash
$skill-installer install --config skills.yaml
```

### Integration with Package Managers

```bash
# Install via npm (if available)
npm install -g @dioxus/agent-skills

# Install via homebrew (macOS)
brew install dioxus-agent-skills

# Install via apt (Ubuntu/Debian)
apt install dioxus-agent-skills
```

## Verification

After installation, verify skills are working:

```bash
# Test basic functionality
$dioxus-project-setup --version

# Create test project
mkdir test-project && cd test-project
$dioxus-project-setup create-basic-app

# Verify all skills
$skill-installer verify
```

## Support

If you encounter issues:

1. Check the [troubleshooting guide](https://github.com/dioxus-community/agent-skills/wiki/Troubleshooting)
2. Search [existing issues](https://github.com/dioxus-community/agent-skills/issues)
3. Join the [Dioxus Discord](https://discord.gg/XgGxMSkvUM) for community support
4. Create a [new issue](https://github.com/dioxus-community/agent-skills/issues/new) with details

The skill installer ensures you have access to the latest Dioxus development capabilities with minimal setup effort.