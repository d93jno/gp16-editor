using GP16Editor.Resources.Styles;
using System;
using System.Collections.Generic;
using System.Linq;

namespace GP16Editor
{
    public static class ThemeManager
    {
        public static void SetTheme(string themeName)
        {
            if (Application.Current == null) return;

            var mergedDictionaries = Application.Current.Resources.MergedDictionaries;
            var themeDictionaries = mergedDictionaries.OfType<ResourceDictionary>().Where(d => d is LightTheme || d is DarkTheme).ToList();

            foreach (var dict in themeDictionaries)
            {
                mergedDictionaries.Remove(dict);
            }

            ResourceDictionary newTheme = themeName switch
            {
                "Light" => new LightTheme(),
                "Dark" => new DarkTheme(),
                _ => GetSystemTheme()
            };

            mergedDictionaries.Add(newTheme);
            
            Application.Current.UserAppTheme = themeName switch
            {
                "Light" => AppTheme.Light,
                "Dark" => AppTheme.Dark,
                _ => AppTheme.Unspecified
            };
        }

        private static ResourceDictionary GetSystemTheme()
        {
            if (Application.Current == null) return new LightTheme(); // Default to light theme
            return Application.Current.RequestedTheme switch
            {
                AppTheme.Light => new LightTheme(),
                AppTheme.Dark => new DarkTheme(),
                _ => new LightTheme()
            };
        }
    }
}
