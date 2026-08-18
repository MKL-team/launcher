//! MKL 核心 Rust 模块（R2 占位）。
//!
//! 规划承载（规格 R2 第三部分）：DownloadService / AuthService / SelfUpdater / 插件沙箱。
//! 当前提供：Minecraft 版本解析与 Java 版本需求映射（供 JavaManager 经 FFI 复用）。
//!
//! FFI 铁律：对外只暴露 `extern "C"` + `#[no_mangle]`，不 panic 越过 FFI 边界。

/// Minecraft 版本号（major.minor.patch）
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct McVersion {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
}

impl McVersion {
    pub fn parse(s: &str) -> Option<McVersion> {
        let mut parts = s.split('.');
        let major = parts.next()?.parse().ok()?;
        let minor = parts.next()?.parse().ok()?;
        let patch = parts.next()?.parse().ok()?;
        Some(McVersion { major, minor, patch })
    }

    /// Minecraft 版本 → 所需 Java 大版本（规格 B9 修正）：
    /// < 1.17 → 8；1.17..1.20.4 → 17；>= 1.20.5 → 21
    pub fn java_requirement(&self) -> u8 {
        if self.major > 1 {
            return 21;
        }
        if self.minor < 17 {
            8
        } else if self.minor < 20 || (self.minor == 20 && self.patch < 5) {
            17
        } else {
            21
        }
    }
}

/// 便捷：按版本字符串返回所需 Java 大版本
pub fn java_requirement_for(mc_version: &str) -> Option<u8> {
    McVersion::parse(mc_version).map(|v| v.java_requirement())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_basic() {
        assert_eq!(McVersion::parse("1.20.4").unwrap().minor, 20);
        assert_eq!(McVersion::parse("1.20.4").unwrap().patch, 4);
        assert!(McVersion::parse("not-a-version").is_none());
        assert!(McVersion::parse("1").is_none());
    }

    #[test]
    fn java_mapping() {
        assert_eq!(java_requirement_for("1.16.5"), Some(8));
        assert_eq!(java_requirement_for("1.12.2"), Some(8));
        assert_eq!(java_requirement_for("1.17.0"), Some(17));
        assert_eq!(java_requirement_for("1.20.4"), Some(17));
        assert_eq!(java_requirement_for("1.20.5"), Some(21));
        assert_eq!(java_requirement_for("1.21.1"), Some(21));
        assert_eq!(java_requirement_for("2.0.0"), Some(21)); // 未来版本按 21
    }
}
