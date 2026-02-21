mod db;
mod state;
mod ui;

use state::{DatabaseViewer, Message};

pub fn main() -> iced::Result {
    iced::application(boot, update, ui::view)
        .title("Postgres Browser")
        .theme(iced::Theme::Dark)
        .run()
}

fn boot() -> (DatabaseViewer, iced::Task<Message>) {
    (
        DatabaseViewer::default(),
        iced::Task::perform(db::init_db(), Message::DatabaseInitialized),
    )
}

fn update(state: &mut DatabaseViewer, message: Message) -> iced::Task<Message> {
    match message {
        Message::InitDatabase => {
            state.loading_tables = true;
            iced::Task::perform(db::init_db(), Message::DatabaseInitialized)
        }
        Message::DatabaseInitialized(result) => {
            state.loading_tables = false;
            match result {
                Ok(pool) => {
                    state.pool = Some(pool);
                    state.error = None;
                    state.loading_tables = true;
                    return iced::Task::perform(
                        db::fetch_tables(state.pool.clone().unwrap()),
                        Message::TablesLoaded,
                    );
                }
                Err(e) => state.error = Some(e),
            }
            iced::Task::none()
        }
        Message::TablesLoaded(result) => {
            state.loading_tables = false;
            match result {
                Ok(tables) => {
                    state.tables = tables;
                    state.error = None;
                }
                Err(e) => state.error = Some(e),
            }
            iced::Task::none()
        }
        Message::ClickTable(table_name) => {
            if let Some(pool) = &state.pool {
                state.loading_details = true;
                iced::Task::perform(
                    db::fetch_table_details(pool.clone(), table_name),
                    Message::TableDetailsLoaded,
                )
            } else {
                iced::Task::none()
            }
        }
        Message::TableDetailsLoaded(result) => {
            state.loading_details = false;
            match result {
                Ok(details) => {
                    state.table_details = Some(details);
                    state.error = None;
                }
                Err(e) => state.error = Some(e),
            }
            iced::Task::none()
        }
        Message::FetchTopRows(table_name) => {
            if let Some(pool) = &state.pool {
                state.loading_top_rows = true;
                iced::Task::perform(
                    db::fetch_top_rows(pool.clone(), table_name),
                    Message::TopRowsLoaded,
                )
            } else {
                iced::Task::none()
            }
        }
        Message::TopRowsLoaded(result) => {
            state.loading_top_rows = false;
            match result {
                Ok(rows) => {
                    if let Some(details) = &mut state.table_details {
                        details.rows = Some(rows);
                    }
                    state.error = None;
                }
                Err(e) => state.error = Some(e),
            }
            iced::Task::none()
        }
    }
}
