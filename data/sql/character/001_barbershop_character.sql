-- ============================================================================
--  Character Barbershop Cleanup (tw_char)
-- ============================================================================

-- Native hair customization utilizes core character appearance fields (playerBytes, playerBytes2).
-- Clean up legacy custom height table if it was previously created.
DROP TABLE IF EXISTS `character_barbershop_look`;
