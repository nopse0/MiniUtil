namespace skyrim_util {

    bool is_playing()
    {
        auto ui = RE::UI::GetSingleton();
        if (!ui) {
            return false;
        }

        // 1. Block if the game is natively paused (Esc menu, inventory, etc.)
        if (ui->GameIsPaused()) {
            return false;
        }

        // 2. Block if an "Application Menu" is open. 
        // This internal engine flag covers the Main Menu (title screen) and Loading screens.
        if (ui->IsApplicationMenuOpen()) {
            return false;
        }

        // 3. SkyUI / MCM Specific Protection (Crucial for Skyrim Souls RE compatibility)
        // The journal menu includes the MCM. Since it might be unpaused by mods, 
        // we explicitly check its presence on the stack.
        if (ui->IsMenuOpen("Journal Menu")) {
            return false;
        }

        // 4. (Optional) Prevent running if the console or full-screen menus are open
        if (ui->IsMenuOpen("Console") || ui->IsMenuOpen("Loading Menu")) {
            return false;
        }

        return true;
    }


}