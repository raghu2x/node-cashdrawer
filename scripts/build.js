const { execSync } = require('child_process');
const os = require('os');

const platform = os.platform();
const arches = platform === 'darwin' ? ['x64', 'arm64'] : ['x64'];

console.log(`Building for ${platform} on architectures: ${arches.join(', ')}`);

for (const arch of arches) {
  console.log(`Building for ${arch}...`);
  try {
    execSync(`prebuildify --napi --strip --name node.napi --arch ${arch}`, {
      stdio: 'inherit'
    });
    console.log(`✅ Successfully built for ${arch}`);
  } catch (error) {
    console.error(`❌ Failed to build for ${arch}:`, error.message);
    process.exit(1);
  }
}

console.log('🎉 All builds completed!');