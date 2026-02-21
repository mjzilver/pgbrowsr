use iced::widget::{button, column, container, row, scrollable, text};
use iced::{Element, Length};
use crate::state::{DatabaseViewer, Message, TableDetails};

pub fn view(state: &'_ DatabaseViewer) -> Element<'_, Message> {
    column![
        view_status_panel(state),
        row![view_tables_panel(state), view_details_panel(state)]
            .spacing(24)
            .height(Length::Fill)
    ]
    .padding(24)
    .spacing(24)
    .into()
}

fn view_status_panel(state: &'_ DatabaseViewer) -> Element<'_, Message> {
    column![]
        .spacing(10)
        .push(if state.pool.is_none() && !state.loading_tables {
            button("Connect to Database").on_press(Message::InitDatabase)
        } else if state.loading_tables || state.loading_details || state.loading_top_rows {
            button("Loading...")
        } else {
            button("Connected")
        })
        .push(if let Some(error) = &state.error {
            text(format!("Error: {}", error)).color([1.0, 0.0, 0.0])
        } else {
            text("")
        })
        .padding(10)
        .into()
}

fn view_tables_panel(state: &'_ DatabaseViewer) -> Element<'_, Message> {
    if state.pool.is_some() {
        let table_buttons = state
            .tables
            .iter()
            .fold(column![].spacing(6), |col, table_name| {
                col.push(
                    button(text(table_name.clone()))
                        .on_press(Message::ClickTable(table_name.clone())),
                )
            });

        container(scrollable(table_buttons).height(Length::Fill))
            .padding(10)
            .width(Length::FillPortion(1))
            .into()
    } else {
        column![text("Tables will appear here")].into()
    }
}

fn top_rows_view(details: &'_ TableDetails) -> Element<'_, Message> {
    let header_row = details
        .columns
        .iter()
        .fold(row![].spacing(10), |r, col| r.push(text(&col.name)));

    let rows_column = details.rows.as_ref().map(|rows| {
        rows.iter()
            .fold(column![].spacing(5).push(header_row), |col, row| {
                let row_view = row
                    .values
                    .iter()
                    .fold(row![].spacing(10), |r, val| r.push(text(val)));
                col.push(row_view)
            })
    });

    let rows_column = rows_column.unwrap_or_else(|| column![text("No rows")].spacing(5));

    scrollable(rows_column).height(Length::Fill).into()
}

fn view_details_panel(state: &'_ DatabaseViewer) -> Element<'_, Message> {
    if let Some(details) = &state.table_details {
        let columns = details
            .columns
            .iter()
            .fold(column![].spacing(5), |col, col_details| {
                col.push(text(format!(
                    "{}: {}{}",
                    col_details.name,
                    col_details.data_type,
                    if col_details.is_nullable {
                        " (nullable)"
                    } else {
                        ""
                    }
                )))
            });

        let top_rows_column = if details.rows.is_some() {
            top_rows_view(details)
        } else if state.loading_top_rows {
            text("Loading top rows...").into()
        } else {
            text("Click 'Fetch Top Rows' to load data").into()
        };

        container(
            column![
                text(format!("Table: {}", details.name)).size(20),
                text(format!("Row count: {}", details.row_count)),
                container(columns).padding(5),
                container(scrollable(top_rows_column))
                    .padding(5)
                    .height(Length::Fill),
                button("Fetch Top Rows").on_press(Message::FetchTopRows(details.name.clone()))
            ]
            .spacing(10)
            .padding(10),
        )
        .width(Length::FillPortion(2))
        .into()
    } else {
        column![text("Select a table to see details")].into()
    }
}
